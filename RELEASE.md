# Release And Branching Workflow

This project uses a `develop -> main` release model with PR-only merges.

## Daily Development Flow

1. Start from latest `develop`:

```bash
git checkout develop
git pull
```

2. Create a feature branch:

```bash
git checkout -b feat/<short-name>
```

3. Work, commit, and push:

```bash
git add .
git commit -m "feat: <what changed>"
git push -u origin feat/<short-name>
```

4. Open PR:

* base: `develop`
* compare: `feat/<short-name>`

5. Merge PR to `develop` after review/testing.

## Release Flow

1. Ensure `develop` is stable and tested.
2. Open PR:

* base: `main`
* compare: `develop`

3. Merge PR.
4. Tag release on `main`:

```bash
git checkout main
git pull
git tag -a vX.Y.Z -m "Release vX.Y.Z"
git push origin vX.Y.Z
```

## Hotfix Flow

1. Branch from `main`:

```bash
git checkout main
git pull
git checkout -b hotfix/<short-name>
```

2. Open PR to `main`.
3. After merge to `main`, back-merge fix to `develop` (PR `main -> develop` or dedicated hotfix PR).

## Branch Naming

* `feat/<name>`: new feature
* `fix/<name>`: bug fix
* `hotfix/<name>`: urgent production fix
* `chore/<name>`: maintenance

## Recommended GitHub Settings (New Project Setup)

When starting a new repo, configure this baseline:

1. Create branches:

```bash
git checkout -b develop
git push -u origin develop
git checkout main
```

2. Add PR template at `.github/PULL_REQUEST_TEMPLATE.md`.
3. Protect `main`:

* Require pull request before merge
* Require at least 1 approval
* Require branch up to date before merge
* Include administrators
* Disable force pushes
* Disable branch deletion

4. Protect `develop` (lighter rules):

* Require pull request before merge
* Optional approval requirement (team preference)

5. Keep direct pushes disabled to `main` (and ideally `develop`).
6. Enable squash merge or merge commit consistently (team choice, but be consistent).

## Versioning Recommendation

Use Semantic Versioning:

* `MAJOR`: breaking change
* `MINOR`: backward-compatible feature
* `PATCH`: backward-compatible bug fix

Example tags:

* `v1.0.0`
* `v1.1.0`
* `v1.1.1`
