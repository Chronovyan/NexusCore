# Branch Protection Rules

This document explains how to configure branch protection rules in GitHub to enforce code quality and testing requirements.

## Setting Up Branch Protection

Branch protection rules prevent certain actions on important branches, such as `master` or `main`, until specific conditions are met.

### Steps to Configure Branch Protection

1. Go to your repository on GitHub
2. Click on "Settings" in the top navigation bar
3. Select "Branches" from the left sidebar
4. Under "Branch protection rules," click "Add rule"
5. In the "Branch name pattern" field, enter the branch name (e.g., `master` or `main`)
6. Configure the following settings:

### Recommended Settings

#### Required Reviews and Approvals
- [x] **Require a pull request before merging**
- [x] **Require approvals** (Recommended: at least 1)
- [x] **Dismiss stale pull request approvals when new commits are pushed**
- [x] **Require review from Code Owners** (if you have a CODEOWNERS file)

#### Required Status Checks
- [x] **Require status checks to pass before merging**
- [x] **Require branches to be up to date before merging**
- [x] **Status checks that are required:**
  - `build-and-test (ubuntu-latest)` 
  - `build-and-test (windows-latest)`

#### Additional Protections
- [x] **Restrict who can push to matching branches** (Recommended for critical branches)
- [x] **Do not allow bypassing the above settings** (Prevents force-pushing or admin overrides)

## Example Configuration Screenshot

Below is an example of a properly configured branch protection rule:

```
[This would be a screenshot showing the branch protection settings page with the recommended settings checked]
```

## Benefits

This configuration ensures:

1. All changes to protected branches go through pull requests
2. Code is reviewed by at least one team member
3. CI tests pass on all supported platforms
4. The branch is up to date with the target branch before merging
5. Nobody can accidentally push directly to protected branches

## Enforcing CI for All Pull Requests

Setting up branch protection as described above ensures that all pull requests must pass the CI smoke tests before they can be merged. This catches issues early and prevents untested or failing code from entering the codebase. 