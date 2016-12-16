#!/bin/bash

if [[ $TRAVIS_OS_NAME == 'osx' ]]; then

    # Install some custom requirements on OS X
    # e.g. brew install pyenv-virtualenv

    case "${TOXENV}" in
        py32)
            # Install some custom Python 3.2 requirements on OS X
            ;;
        py33)
            # Install some custom Python 3.3 requirements on OS X
            ;;
        py34)
            # Install some custom Python 3.3 requirements on OS X
            ;;
        py35)
            # Install some custom Python 3.3 requirements on OS X
            ;;
    esac
else
    # Install some custom requirements on Linux
fi