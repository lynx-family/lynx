import { LynxError } from './lynx-error';

export abstract class LynxViewClient {
  public onReceivedError(error: LynxError): void {}
}
