# -*- coding: UTF-8 -*-
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import json
import time
from collections import namedtuple
from urllib.parse import quote

from appium.webdriver.common.appiumby import AppiumBy
from lynx_e2e.api.lynx_view import LynxView
from lynx_e2e.api.upath import UPath


RAW_PARENT_URL = (
    "file://lynx?local://automation/nav-basic/main.lynx.bundle"
    "?nav_role=parent"
)
CANONICAL_PARENT_URL = (
    "hybrid://lynxview_page"
    "?bundle=automation%2Fnav-basic%2Fmain.lynx.bundle"
    "&nav_role=parent"
)
MALFORMED_CANONICAL_URL = "hybrid://lynxview_page?nav_role=parent"
MAPPED_LEGACY_URL = (
    "file://lynx?local://automation/nav-basic/main.lynx.bundle"
    "?nav_role=parent"
    "&title=Mapped%20Title"
    "&hidden_nav=yes"
    "&fullscreen=no"
    "&back_button_style=dark"
    "&initial_page=details"
    "&custom_flag=preserved"
    "&theme=page-theme"
)
EXPLORER_BUNDLE_ID = "com.lynx.LynxExplorer"

HOMEPAGE_CONFIG = {
    "type": "custom",
    "path": "homepage",
    "platform": "ios",
    "restart_before_exec": True,
}

_NativeContext = namedtuple(
    "_NativeContext",
    ("view", "target", "anchor_tag", "anchor", "anchor_texts"),
)
_LynxViewRect = namedtuple(
    "_LynxViewRect",
    ("left", "top", "width", "height"),
)

_DISPLAY_ANCHORS = (
    # This marker includes the parent/child role, so it disambiguates two
    # nav-basic sessions whose LynxViews occupy the same native frame.
    "nav-page-marker",
    # The homepage input can temporarily be outside WDA's displayed viewport
    # after relaunching with a persisted Sparkling preference. The segmented
    # runtime controls remain at the top and have stable native identifiers,
    # so they provide an independent homepage/session correlation anchor.
    "runtime-lynx",
    "runtime-sparkling",
    "bundle-url-input",
)


def send_cdp(test, session_id, method, params=None):
    request_id = test.app.send_cdp_data(
        session_id=session_id,
        method=method,
        params=params or {},
    )
    raw_response = test.app.wait_for_cdp_id(request_id, raw=True)
    response = json.loads(raw_response["message"])
    if "error" in response:
        raise RuntimeError("%s failed: %s" % (method, response["error"]))
    return response.get("result", {})


def open_external_url(test, url):
    """Deliver a route through Explorer's registered UIApplication scheme."""
    registered_url = "lynx://open?url=%s" % quote(url, safe="")
    test.app._appium_driver.execute_script(
        "mobile: deepLink",
        {
            "url": registered_url,
            "bundleId": EXPLORER_BUNDLE_ID,
        },
    )


def accept_route_error_alert(test, expected_code, timeout=10):
    deadline = time.time() + timeout
    last_error = None
    while time.time() < deadline:
        try:
            alert = test.app._appium_driver.switch_to.alert
        except Exception as error:
            last_error = error
            time.sleep(0.2)
            continue

        alert_text = alert.text
        if expected_code not in alert_text:
            raise AssertionError(
                "Expected route error code %r, got alert %r"
                % (expected_code, alert_text)
            )
        alert.accept()
        return

    detail = " (%s)" % last_error if last_error is not None else ""
    raise AssertionError(
        "Timed out waiting for route error alert %r%s"
        % (expected_code, detail)
    )


def _xpath_literal(value):
    if "'" not in value:
        return "'%s'" % value
    if '"' not in value:
        return '"%s"' % value
    pieces = value.split("'")
    return "concat(%s)" % ", \"'\", ".join(
        "'%s'" % piece for piece in pieces
    )


def _is_displayed(element):
    try:
        return bool(element.is_displayed())
    except Exception:
        return False


def _native_tag_elements(native_view, tag):
    # XCUITest exposes accessibilityIdentifier as `name`. For elements that do
    # not set an identifier, WebDriverAgent also falls back to accessibilityLabel,
    # which is where Lynx publishes lynx-test-tag in dev builds.
    xpath = ".//*[@name=%s]" % _xpath_literal(tag)
    return native_view.find_elements(AppiumBy.XPATH, xpath)


def _native_text_values(element):
    values = set()
    try:
        value = element.text
        if isinstance(value, str) and value:
            values.add(value)
    except Exception:
        pass
    # WDA does not expose a `text` attribute on iOS. The Selenium `.text`
    # property above is the supported path; asking get_attribute("text")
    # generates a caught 500 for every session probe.
    for attribute in ("value", "label", "name"):
        try:
            value = element.get_attribute(attribute)
        except Exception:
            continue
        if isinstance(value, str) and value:
            values.add(value)
    return frozenset(values)


def _displayed_anchor(native_view, requested_tag):
    ordered_tags = list(_DISPLAY_ANCHORS)
    if requested_tag not in ordered_tags:
        ordered_tags.append(requested_tag)
    for tag in ordered_tags:
        for element in _native_tag_elements(native_view, tag):
            if _is_displayed(element):
                return tag, element
    return None


def _visible_native_contexts(test, requested_tag):
    """Return displayed Appium LynxViews with a stable native anchor.

    lynx-e2e-appium 0.0.12's iOS get_lynxview() always chooses elements[0].
    That is unsafe after a UIKit push because the old LynxView can remain in
    the accessibility tree. Query the pinned driver's native tree directly and
    require a displayed page anchor instead. Some non-interactive Lynx nodes
    publish ``lynx-test-tag`` only through CDP, so the requested tag itself is
    intentionally validated later by ``_probe_session``.
    """
    appium_driver = test.app._appium_driver
    native_views = appium_driver.find_elements(
        AppiumBy.XPATH,
        "//*[@label='lynxview']",
    )
    contexts = []
    for native_view in native_views:
        if not _is_displayed(native_view):
            continue
        anchor_match = _displayed_anchor(native_view, requested_tag)
        if anchor_match is None:
            continue
        anchor_tag, anchor = anchor_match
        contexts.append(
            _NativeContext(
                view=native_view,
                target=None,
                anchor_tag=anchor_tag,
                anchor=anchor,
                anchor_texts=_native_text_values(anchor),
            )
        )
    return contexts


def _session_ids(test):
    session_ids = []
    seen = set()
    for session in test.app.get_session_list() or ():
        if not isinstance(session, dict) or "session_id" not in session:
            continue
        session_id = session["session_id"]
        key = str(session_id)
        if key in seen:
            continue
        seen.add(key)
        session_ids.append(session_id)
    return session_ids


def _request_session_refresh(test):
    # SessionList is asynchronous. A failed refresh is harmless because the
    # polling loop will continue using the latest list already held by E2E.
    try:
        test.app.send_custom_data("ListSession", {})
    except Exception:
        pass


def _rect_values(rect):
    if isinstance(rect, dict):
        return (
            float(rect["x"]),
            float(rect["y"]),
            float(rect["width"]),
            float(rect["height"]),
        )
    return (
        float(rect.left),
        float(rect.top),
        float(rect.width),
        float(rect.height),
    )


def _rects_correspond(native_rect, cdp_rect):
    native_x, native_y, native_width, native_height = _rect_values(native_rect)
    cdp_x, cdp_y, cdp_width, cdp_height = _rect_values(cdp_rect)
    if min(native_width, native_height, cdp_width, cdp_height) <= 0:
        return False

    native_center = (
        native_x + native_width / 2,
        native_y + native_height / 2,
    )
    cdp_center = (cdp_x + cdp_width / 2, cdp_y + cdp_height / 2)
    # Native (Appium, points) and CDP (DevTool, CSS px) geometry never match
    # exactly: sub-pixel rounding, safe-area insets and status-bar offsets drift
    # by a few px. Tolerances are relative to element size (so large elements
    # allow more absolute drift) with an empirical floor for tiny elements:
    #   position: 15% of the smaller side, min 12px  (center alignment)
    #   size:     25% of the largest side, min 16px  (width/height match)
    # These floors/ratios were tuned against the audited route entries; loosen
    # only if a real, correctly-rendered element starts failing.
    position_tolerance = max(
        12.0,
        min(
            max(native_width, cdp_width),
            max(native_height, cdp_height),
        )
        * 0.15,
    )
    size_tolerance = max(
        16.0,
        max(native_width, native_height, cdp_width, cdp_height) * 0.25,
    )
    return (
        abs(native_center[0] - cdp_center[0]) <= position_tolerance
        and abs(native_center[1] - cdp_center[1]) <= position_tolerance
        and abs(native_width - cdp_width) <= size_tolerance
        and abs(native_height - cdp_height) <= size_tolerance
    )


def _tag_ids(driver, tag):
    path = UPath(
        predicates=[
            {
                "name": "lynx-test-tag",
                "value": tag,
                "operator": "==",
            }
        ]
    )
    return driver.get_element_ids(path)


def _bound_view(test, native_view, session_id):
    """Create a LynxView whose driver is locked to one explicit session.

    In lynx-e2e-appium 0.0.12, LynxDriver.get_session_id() ignores the view and
    returns the largest global session ID. Set the driver's target before its
    first DOM request so every subsequent lookup on this view stays bound.
    """
    # lynx-e2e-appium 0.0.15 models a LynxView by its native screen rect and
    # no longer exposes get_page_root(). Keep the displayed Appium view only
    # as the source of that rect, then bind the wrapper to the exact DevTool
    # session below.
    lynxview = LynxView(
        _LynxViewRect(*_rect_values(native_view.rect)),
        app=test.app,
    )
    driver = test.app.get_lynx_driver(lynxview)
    driver._session_id = session_id
    lynxview._lynx_driver = driver
    return lynxview, driver


def _probe_session(test, context, session_id, requested_tag, expected_text):
    lynxview, driver = _bound_view(test, context.view, session_id)

    target_ids = _tag_ids(driver, requested_tag)
    if len(target_ids) != 1:
        return None
    target_text = driver.get_text(target_ids[0])
    if expected_text is not None and target_text != expected_text:
        return None

    anchor_ids = _tag_ids(driver, context.anchor_tag)
    if len(anchor_ids) != 1:
        return None
    anchor_id = anchor_ids[0]
    anchor_text = driver.get_text(anchor_id)
    if (
        context.anchor_texts
        and anchor_text
        and anchor_text not in context.anchor_texts
    ):
        return None
    if not _rects_correspond(context.anchor.rect, driver.get_rect(anchor_id)):
        return None

    return lynxview, target_text


def _resolve_visible_view_once(test, tag, expected_text=None):
    contexts = _visible_native_contexts(test, tag)
    if not contexts:
        raise AssertionError(
            "No displayed native LynxView has an anchor for tag %r" % tag
        )

    session_ids = _session_ids(test)
    if not session_ids:
        raise AssertionError("DevTool reported no Lynx sessions")

    matches = []
    errors = []
    for context in contexts:
        for session_id in session_ids:
            try:
                match = _probe_session(
                    test,
                    context,
                    session_id,
                    tag,
                    expected_text,
                )
            except Exception as error:
                errors.append("%s: %s" % (session_id, error))
                continue
            if match is not None:
                matches.append((session_id, match[0], match[1]))

    if len(matches) == 1:
        _, lynxview, actual_text = matches[0]
        return lynxview, actual_text
    if len(matches) > 1:
        raise AssertionError(
            "Ambiguous visible LynxView/session binding for tag %r: %r"
            % (tag, [match[0] for match in matches])
        )
    detail = "; ".join(errors[-3:]) if errors else "no session matched"
    raise AssertionError(
        "No DevTool session matched displayed native tag %r (%s)"
        % (tag, detail)
    )


def wait_for_visible_view(test, tag, expected_text=None, timeout=20):
    """Bind the displayed Appium LynxView to its matching DevTool session.

    UIKit keeps previous LynxViews alive across navigation. Each polling pass
    therefore validates native visibility, native tag ownership, the displayed
    anchor's text/frame, and the candidate session DOM before returning.
    """
    deadline = time.time() + timeout
    last_error = None
    last_text = None
    while time.time() < deadline:
        _request_session_refresh(test)
        try:
            lynxview, last_text = _resolve_visible_view_once(
                test,
                tag,
                expected_text,
            )
            return lynxview
        except Exception as error:
            last_error = error
        time.sleep(0.2)

    detail = "last text=%r" % last_text
    if last_error is not None:
        detail += ", last error=%s" % last_error
    raise AssertionError(
        "Timed out waiting for displayed tag %r with text %r (%s)"
        % (tag, expected_text, detail)
    )


def tag_text(lynxview, tag):
    element = lynxview.get_by_test_tag(tag)
    element.wait_for_existing(2)
    return element.text


def wait_for_bound_tag_text(lynxview, tag, expected_text, timeout=10):
    """Wait for a rerender without rebinding the displayed LynxView session."""
    deadline = time.time() + timeout
    last_text = None
    last_error = None
    while time.time() < deadline:
        try:
            last_text = tag_text(lynxview, tag)
            if last_text == expected_text:
                return
        except Exception as error:
            # A ReactLynx state update can briefly replace the tagged node.
            last_error = error
        time.sleep(0.2)

    detail = ""
    if last_error is not None:
        detail = ", last error=%s" % last_error
    raise AssertionError(
        "Timed out waiting for bound tag %r to become %r "
        "(last text=%r%s)"
        % (tag, expected_text, last_text, detail)
    )


def click_visible_tag(lynxview, tag, timeout=10):
    xpath = "//*[@name=%s]" % _xpath_literal(tag)
    deadline = time.time() + timeout
    match_count = 0
    displayed_count = 0
    last_click_error = None
    while time.time() < deadline:
        native_matches = lynxview.app._appium_driver.find_elements(
            AppiumBy.XPATH,
            xpath,
        )
        match_count = len(native_matches)
        displayed_matches = [
            element
            for element in native_matches
            if _is_displayed(element)
        ]
        displayed_count = len(displayed_matches)
        click_target = None
        if displayed_count == 1:
            # ReactLynx can replace the accessibility subtree after
            # Input.insertText. Click the freshly queried native element
            # instead of retaining a stale child from the bound LynxView.
            click_target = displayed_matches[0]
        elif match_count == 1:
            # WDA reports scroll-view descendants outside the current viewport
            # as not displayed. XCUIElement.tap, which backs Appium's native
            # click, scrolls a unique off-screen descendant into view first.
            # Retain the displayed-only rule when multiple pages expose the
            # same tag so a covered LynxView can never win by accident.
            click_target = native_matches[0]
        if click_target is not None:
            try:
                click_target.click()
                return
            except Exception as error:
                # The accessibility subtree may be replaced between lookup and
                # tap. Requery globally on the next polling pass.
                last_click_error = error
        time.sleep(0.2)

    detail = ""
    if last_click_error is not None:
        detail = ", last click error=%s" % last_click_error
    raise AssertionError(
        (
            "Expected one unambiguous native tag %r, found %d total / "
            "%d displayed%s"
        )
        % (tag, match_count, displayed_count, detail)
    )


def dismiss_software_keyboard(test, timeout=5):
    """Dismiss iOS input and wait until WDA reports the viewport restored."""
    driver = test.app._appium_driver
    try:
        if not driver.is_keyboard_shown():
            return
        # The homepage input has the default Return key. Passing it lets
        # XCUITest resign first responder deterministically instead of relying
        # on an arbitrary tap that may hit one of the route buttons.
        driver.hide_keyboard(key_name="return")
    except Exception as error:
        raise AssertionError(
            "Unable to dismiss the software keyboard: %s" % error
        ) from error

    deadline = time.time() + timeout
    last_error = None
    while time.time() < deadline:
        try:
            if not driver.is_keyboard_shown():
                return
        except Exception as error:
            last_error = error
        time.sleep(0.2)

    detail = " (%s)" % last_error if last_error is not None else ""
    raise AssertionError(
        "Timed out waiting for the software keyboard to disappear%s" % detail
    )


def assert_tag_text(test, tag, expected, timeout=15):
    lynxview = wait_for_visible_view(test, tag, expected, timeout)
    actual = tag_text(lynxview, tag)
    if actual != expected:
        raise AssertionError(
            "Expected tag %r to be %r, got %r" % (tag, expected, actual)
        )
    return lynxview


def assert_nonempty_tag(test, tag, forbidden=None, timeout=15):
    forbidden_values = set(forbidden or ())
    deadline = time.time() + timeout
    last_value = None
    while time.time() < deadline:
        lynxview = wait_for_visible_view(test, tag, timeout=2)
        last_value = tag_text(lynxview, tag)
        if last_value and last_value not in forbidden_values:
            return last_value
        time.sleep(0.2)
    raise AssertionError(
        "Expected tag %r to be nonempty and not in %r, got %r"
        % (tag, forbidden_values, last_value)
    )


def assert_xelement_input(test, timeout=15):
    """Prove the Sparkling page instantiated XElement's native input UI."""
    wait_for_visible_view(test, "nav-xelement-input", timeout=timeout)
    xpath = "//*[@name='nav-xelement-input']"
    deadline = time.time() + timeout
    last_types = []
    while time.time() < deadline:
        matches = test.app._appium_driver.find_elements(AppiumBy.XPATH, xpath)
        displayed = [element for element in matches if _is_displayed(element)]
        last_types = [element.get_attribute("type") for element in displayed]
        if len(displayed) == 1 and last_types == ["XCUIElementTypeTextField"]:
            return
        time.sleep(0.2)
    raise AssertionError(
        "Expected one displayed native XElement text field, found types %r"
        % last_types
    )


def open_from_homepage(test, url, button_tag, runtime=None):
    homepage = wait_for_visible_view(test, "bundle-url-input")
    if runtime is not None:
        if runtime not in ("lynx", "sparkling"):
            raise ValueError("Unsupported homepage runtime %r" % runtime)
        click_visible_tag(homepage, "runtime-%s" % runtime)
        runtime_label = (
            "Open with Sparkling" if runtime == "sparkling" else "Open with Lynx"
        )
        # Selecting a runtime rerenders the homepage in place. Keep using the
        # already-bound DevTool session: trying to rediscover it by correlating
        # the native accessibility tree during that rerender races SessionList
        # refresh and can reject the otherwise valid homepage session.
        wait_for_bound_tag_text(
            homepage,
            "bundle-runtime-label",
            runtime_label,
        )
    input_element = homepage.get_by_test_tag("bundle-url-input")
    session_id = homepage.get_session_id()
    send_cdp(test, session_id, "DOM.focus", {"nodeId": input_element.id})
    send_cdp(test, session_id, "Input.insertText", {"text": url})
    # The route buttons remain visible above the iOS keyboard. Tapping the
    # intended action is both closer to the user flow and more portable than
    # requiring Appium's Return-key shortcut to resign a ReactLynx input.
    click_visible_tag(homepage, button_tag)
    return wait_for_visible_view(test, "nav-role", "parent")


def assert_legacy_capabilities(test, lynxview=None):
    expected_tags = (
        "nav-container-type",
        "nav-container-id",
        "nav-sparkling-navigation",
        "nav-spk-pipe",
    )
    for tag in expected_tags:
        if lynxview is None:
            assert_tag_text(test, tag, "absent")
        else:
            # open_from_homepage already resolved the displayed page and its
            # exact DevTool session. Reuse that binding: a subsequent global
            # SessionList refresh can briefly omit an ordinary Lynx page even
            # though the already-bound session remains usable.
            wait_for_bound_tag_text(lynxview, tag, "absent")


def assert_sparkling_capabilities(test):
    assert_tag_text(test, "nav-container-type", "sparkling")
    assert_nonempty_tag(test, "nav-container-id", forbidden={"absent"})
    assert_nonempty_tag(
        test,
        "nav-lynx-sdk-version",
        forbidden={"absent", "unknown", "unavailable"},
    )
    # Lynx iOS serializes native BOOL global props into JS as numeric 1/0.
    assert_tag_text(test, "nav-sparkling-navigation", "1")
    assert_tag_text(test, "nav-spk-pipe", "available")
    assert_xelement_input(test)
