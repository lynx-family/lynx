# Contributing to Lynx

First off, thank you for considering contributing to Lynx! We welcome you to join and become a member of Lynx Authors. It's people like you who make this project great.

## How Can I Contribute?

### Reporting Bugs

If you find a bug, please open an issue with the following details:
- A clear and descriptive title for the issue.
- A description of the steps to reproduce the issue.
- Any additional information or screenshots that might help us understand the issue better.

### Suggesting Enhancements

We’re always open to new ideas! If you have a suggestion, please:
- Use the “Feature Request” issue template or create a new issue.
- Describe the enhancement you’d like and explain why it would be useful.

### Your First Code Contribution

Unsure where to start? You can find beginner-friendly issues by looking for the “good first issue” label. Working on these issues helps you get familiar with the codebase before tackling more complex problems.

### Pull Requests

When you’re ready to make a code change, please create a pull request:
1. Fork the repository and clone it to your local machine.
2. Create a new branch: `git checkout -b name-of-your-branch`.
3. Make your changes.
4. Once you have finished the necessary tests and verifications locally, commit the changes with a commit message in the following format (some parts are optional):
    ```shell
    [Label] Title of the commit message (one line)

    Summary of change:
    Longer description of change addressing as appropriate: why the change
    is made, context if it is part of many changes, description of previous
    behavior and newly introduced differences, etc.

    Long lines should be wrapped to 72 columns for easier log message
    viewing in terminals.

    issue: #xxx
    doc: https://xxxxxxxx
    TEST: Test cases
    ```
    Some parts in the message template are required, while others are optional. We use the labels `[Required]` and `[Optional]` to differentiate them in the detailed explanation below:
    - **[Required]** The first line of the commit message should be the title, summarizing the changes (the title must be on a separate line).
    - **[Required]** The title must start with at least one label, and the first label must be one of the following: `Feature`, `BugFix`, `Refactor`, `Optimize`, `Infra`, `Testing`, `Doc`. The format should be: `[Label]`, e.g., `[BugFix]`, `[Feature]`, `[BugFix][Tools]` (there must be at least one space between the label(s) and the title content, and the title must not be empty).
        >
        > Which label should I use? Here are the explanations for them:
        > - **`[Feature]`**: New features, new functions, or changes to existing features and functions. For example:
        >   - `[Feature] Add new API for data binding` Add a new API for data binding
        >   - `[Feature] Add service for light sensors` Add a service for light sensors
        >   - `[Feature][Log] Support async event report` Support asynchronous event reporting
        > - **`[BugFix]`**: Fixes for functional defects, performance anomalies, and problems in developer tools, etc. For example:
        >   - `[BugFix] Fix exception when playing audio` Fix the exception when playing audio
        >   - `[BugFix] Fix leaks in xxx` Fix the memory leak problem in xxx
        >   - `[BugFix][DevTool] Fix data error in DevTool` Fix the data error in the debug tool
        > - **`[Refactor]`**: The overall refactoring of a module or function (mainly refers to large-scale code rewriting or architecture optimization; small-scale refactoring can be classified as Optimize). For example:
        >   - `[Refactor][Memory] Memory management in xxxx` Refactor the memory management of the xxx module
        >   - `[Refactor][TestBench] XX module in TestBench` Refactor the xxx module in the TestBench
        > - **`[Optimize]`**: Small-scale optimization of a certain feature or indicator, such as performance optimization, memory optimization, etc. For example:
        >   - `[Optimize][Performance] Jank when scrolling in xxxx` Optimization of the smoothness when scrolling in xxxx
        > - **`[Infra]`**: Changes related to the compilation framework, CQ configuration, basic tools, etc. For example:
        >   - `[Infra][Compile] Use -Oz compile params in xxx` Use -Oz compilation parameters
        > - **`[Testing]`**: Modifications related to test cases and test frameworks. For example:
        >   - `[Testing][Android] Fix xxx test case for Android` Fix a test case for Android
        >   - `[Testing] Optimize test process` Optimize the process of a certain test framework
        >
        > Modifications only involving test cases, even if it is a `BugFix`, should be classified as `Testing`.
        > If both `Feature` code and related test cases are submitted in the same patch, it should be classified as `Feature`.
    - **[Required]** The section following the title should be the summary, providing a detailed description of the changes (there must be a blank line between the title and the summary).
    - **[Required]** The commit must be linked to an issue, and the issue ID needs to appear in the format `issue: #xxx`.
    - **[Optional]** The commit can be linked to a document. If you labeled your changes as `Feature` or `Refactor`, this is required. The format of a doc link should be like this: `doc: https://xxxxxxx`
    - **[Optional]** The commit can be linked to tests (unit tests, UI tests). You can write the case names in the format: `TEST: test_case_1, test_case_2`
    <br>
    We have set up a CI job to ensure that the commit message meets our formatting requirements. So please make sure your message is well formatted before starting the pull request process.
5. Push the changes to your remote branch and start a pull request.
> We encourage the submission of small patches and only accept PRs that contain a single commit. Therefore, please split your PR into separate ones if it contains multiple commits, or squash them into a single commit if there are not too many changes.
> The CI will reject any PR that contains more than one commit.
6. Make sure that your pull request adheres to the style guide and is properly documented.

## Verifying and Reviewing Pull Requests

A pull request needs to be verified by the CI jobs and reviewed by the Lynx authors before being merged.

Once you submit a pull request, you can invite the contributors of the repository to review your changes. If you have no idea whom to invite to review your changes, the Github branch protection rules and `git blame` are the right places to start.

While any contributor can review your changes, at least one of the authors from [default reviewers](./DEFAULT_REVIEWERS) should be on the reviewer list. Default reviewers can help trigger the CI workflow run to verify the changes (if this is your first PR for Lynx, you'll see a pending approval on the PR discussion area after you submit the PR) and then start the landing process. (The reason why the workflow run needs to be triggered by default reviewers is that they need to make sure the new changes will not introduce any risks)

Typically, a pull request will be reviewed **within one week**. The landing process will be triggered manually if the changes pass all CI checks and are ready to be merged.

## Landing Pull Requests

To make sure that new changes won't break the additional tests, all the pull requests need to be landed by a self-hosted CI system. When a pull request is ready to be merged, default reviewers will comment the command `/land` on the PR. Then the CI system will start the landing process running full tests. Please be patient, as that could take a while.

If a failure occurs during the landing process, we have two approaches to handle different situations:
- If a change that breaks the tests is reasonable, the default reviewers will fix them in the self-hosted codebase and restart the failed process. In this situation, the PR author needs to do nothing except wait for it to be merged.
- If the changes are found to have bugs during testing, the person who started the landing process is responsible for providing feedback on the issue and relevant information to the author of the PR. Once the bugs are fixed, the author can restart from verifying and reviewing the PR and then wait for someone to reland it.

When your changes pass all checks in the self-hosted CI system, the PR will be merged automatically.

## Code Style

Our project adheres to the coding style guidelines provided by Google.
You can find the detailed guidelines here: [Google's Style Guides](https://google.github.io/styleguide/).

Following these style guides helps ensure that our code is consistent, clear, and of high quality.
Please make sure to familiarize yourself with these guidelines of C++, Java, Objective-C and Python before contributing to the project.

Special thanks to Google for making these comprehensive style guides available for developers!

## Testing

Make sure your changes are covered by tests, if applicable. Run the existing tests to ensure everything works as expected:
- [Unit Tests](testing/README_UT.md)

## Code of Conduct

Please note that all participants in this project are expected to uphold our [Code of Conduct](CODE_OF_CONDUCT.md). By participating, you agree to abide by its terms.


We're excited to see your contributions! Thank you!