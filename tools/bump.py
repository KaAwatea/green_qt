#!/usr/bin/env python3
from argparse import ArgumentParser
from datetime import date
from git import Repo
from semver import VersionInfo


if __name__ == '__main__':
    parser = ArgumentParser()
    parser.add_argument('--major', type=int, required=True)
    parser.add_argument('--minor', type=int, required=True)
    parser.add_argument('--patch', type=int, required=True)

    args = parser.parse_args()

    repo = Repo('.')
    print("Currently active branch: {}".format(repo.active_branch))

    next_version = VersionInfo(args.major, args.minor, args.patch)

    with open('version.pri') as file:
        major = int(next(file).split('=')[1])
        minor = int(next(file).split('=')[1])
        patch = int(next(file).split('=')[1])
        current_version = VersionInfo(major, minor, patch)

    assert current_version < next_version

    with open('CHANGELOG.md') as file:
        changelog = file.read().replace('## [Unreleased]', '## [Unreleased]\n### Added\n\n### Changed\n\n### Fixed\n\n## [{}] - {}'.format(current_version, date.today().strftime('%Y-%m-%d')))
    with open('CHANGELOG.md', 'w') as file:
        file.write(changelog)

    repo.git.add('CHANGELOG.md')
    repo.git.commit('-m', 'Close version {} in CHANGELOG.md'.format(current_version))

    with open('version.pri', 'w') as file:
        file.write('VERSION_MAJOR={}\n'.format(next_version.major))
        file.write('VERSION_MINOR={}\n'.format(next_version.minor))
        file.write('VERSION_PATCH={}\n'.format(next_version.patch))

    repo.git.add('version.pri')
    repo.git.commit('-m', 'Bump to version {}'.format(next_version))

    print('version updated', current_version, '->', next_version)
