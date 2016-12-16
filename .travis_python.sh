#!/bin/bash

if [[ $TRAVIS_OS_NAME == 'osx' ]]; then
    sw_vers
    brew update || brew update

    brew outdated openssl || brew upgrade openssl

    # install pyenv
    git clone --depth 1 https://github.com/yyuu/pyenv.git ~/.pyenv
    PYENV_ROOT="$HOME/.pyenv"
    PATH="$PYENV_ROOT/bin:$PATH"
    eval "$(pyenv init -)"

    case "${TOXENV}" in
        py33)
            pyenv install 3.3.6
            pyenv global 3.3.6
            ;;
        py34)
            pyenv install 3.4.5
            pyenv global 3.4.5
            ;;
        py35)
            pyenv install 3.5.2
            pyenv global 3.5.2
            ;;
    esac
    pyenv rehash
    python -m pip install --user virtualenv
fi
