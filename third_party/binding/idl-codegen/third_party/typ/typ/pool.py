# Copyright 2014 Google Inc. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import copy
import multiprocessing
import pickle
import traceback

from typ.host import Host


def make_pool(host, jobs, callback, context, pre_fn, post_fn):
    _validate_args(context, pre_fn, post_fn)
    if jobs > 1:
        return _ProcessPool(host, jobs, callback, context, pre_fn, post_fn)
    else:
        return _AsyncPool(host, jobs, callback, context, pre_fn, post_fn)


class _MessageType(object):
    Request = 'Request'
    Response = 'Response'
    Close = 'Close'
    Done = 'Done'
    Error = 'Error'
    Interrupt = 'Interrupt'

    values = [Request, Response, Close, Done, Error, Interrupt]


def _validate_args(context, pre_fn, post_fn):
    try:
        _ = pickle.dumps(context)
    except Exception as e:
        raise ValueError('context passed to make_pool is not picklable: %s'
                         % str(e))
    try:
        _ = pickle.dumps(pre_fn)
    except pickle.PickleError:
        raise ValueError('pre_fn passed to make_pool is not picklable')
    try:
        _ = pickle.dumps(post_fn)
    except pickle.PickleError:
        raise ValueError('post_fn passed to make_pool is not picklable')


class _ProcessPool(object):

    def __init__(self, host, jobs, callback, context, pre_fn, post_fn):
        self.host = host
        self.jobs = jobs
        self.requests = multiprocessing.Queue()
        self.responses = multiprocessing.Queue()
        self.workers = []
        self.discarded_responses = []
        self.closed = False
        self.erred = False
        for worker_num in range(1, jobs + 1):
            w = multiprocessing.Process(target=_loop,
                                        args=(self.requests, self.responses,
                                              host.for_mp(), worker_num,
                                              callback, context,
                                              pre_fn, post_fn))
            w.start()
            self.workers.append(w)

    def send(self, msg):
        self.requests.put((_MessageType.Request, msg))

    def get(self):
        msg_type, resp = self.responses.get()
        if msg_type == _MessageType.Error:
            self._handle_error(resp)
        elif msg_type == _MessageType.Interrupt:
            raise KeyboardInterrupt
        assert msg_type == _MessageType.Response
        return resp

    def close(self):
        for _ in self.workers:
            self.requests.put((_MessageType.Close, None))
        self.closed = True

    def join(self):
        # TODO: one would think that we could close self.requests in close(),
        # above, and close self.responses below, but if we do, we get
        # weird tracebacks in the daemon threads multiprocessing starts up.
        # Instead, we have to hack the innards of multiprocessing. It
        # seems likely that there's a bug somewhere, either in this module or
        # in multiprocessing.
        # pylint: disable=protected-access
        if self.host.is_python3:  # pragma: python3
            multiprocessing.queues.is_exiting = lambda: True
        else:  # pragma: python2
            multiprocessing.util._exiting = True

        if not self.closed:
            # We must be aborting; terminate the workers rather than
            # shutting down cleanly.
            for w in self.workers:
                w.terminate()
                w.join()
            return []

        final_responses = []
        error = None
        interrupted = None
        for w in self.workers:
            while True:
                msg_type, resp = self.responses.get()
                if msg_type == _MessageType.Error:
                    error = resp
                    break
                if msg_type == _MessageType.Interrupt:
                    interrupted = True
                    break
                if msg_type == _MessageType.Done:
                    final_responses.append(resp[1])
                    break
                self.discarded_responses.append(resp)

        for w in self.workers:
            w.join()

        # TODO: See comment above at the beginning of the function for
        # why this is commented out.
        # self.responses.close()

        if error:
            self._handle_error(error)
        if interrupted:
            raise KeyboardInterrupt
        return final_responses

    def _handle_error(self, msg):
        worker_num, tb = msg
        self.erred = True
        raise Exception("Error from worker %d (traceback follows):\n%s" %
                        (worker_num, tb))


# 'Too many arguments' pylint: disable=R0913

def _loop(requests, responses, host, worker_num,
          callback, context, pre_fn, post_fn, should_loop=True):
    host = host or Host()
    try:
        context_after_pre = pre_fn(host, worker_num, context)
        keep_looping = True
        while keep_looping:
            message_type, args = requests.get(block=True)
            if message_type == _MessageType.Close:
                responses.put((_MessageType.Done,
                               (worker_num, post_fn(context_after_pre))))
                break
            assert message_type == _MessageType.Request
            resp = callback(context_after_pre, args)
            responses.put((_MessageType.Response, resp))
            keep_looping = should_loop
    except KeyboardInterrupt as e:
        responses.put((_MessageType.Interrupt, (worker_num, str(e))))
    except Exception as e:
        responses.put((_MessageType.Error,
                       (worker_num, traceback.format_exc(e))))


class _AsyncPool(object):

    def __init__(self, host, jobs, callback, context, pre_fn, post_fn):
        self.host = host or Host()
        self.jobs = jobs
        self.callback = callback
        self.context = copy.deepcopy(context)
        self.msgs = []
        self.closed = False
        self.post_fn = post_fn
        self.context_after_pre = pre_fn(self.host, 1, self.context)
        self.final_context = None

    def send(self, msg):
        self.msgs.append(msg)

    def get(self):
        return self.callback(self.context_after_pre, self.msgs.pop(0))

    def close(self):
        self.closed = True
        self.final_context = self.post_fn(self.context_after_pre)

    def join(self):
        if not self.closed:
            self.close()
        return [self.final_context]
