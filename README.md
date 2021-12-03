## Microservice to record Envelopes from an OD4Session

This repository provides source code for a microservice to record Envelopes
from a running OD4Session.

[![License: GPLv3](https://img.shields.io/badge/license-GPL--3-blue.svg
)](https://www.gnu.org/licenses/gpl-3.0.txt)


## Table of Contents
* [Dependencies](#dependencies)
* [Usage](#usage)
* [Build from sources on the example of Ubuntu 16.04 LTS](#build-from-sources-on-the-example-of-ubuntu-1604-lts)
* [License](#license)


## Dependencies
You need a C++14-compliant compiler to compile this project.

The following dependency is part of the source distribution:
* [libcluon](https://github.com/chrberger/libcluon) - [![License: GPLv3](https://img.shields.io/badge/license-GPL--3-blue.svg
)](https://www.gnu.org/licenses/gpl-3.0.txt)


## Usage
To run this microservice using `docker-compose`, you can simply add the following
section to your `docker-compose.yml` to record Envelopes from CID `111`:

```yml
version: '2' # Must be present exactly once at the beginning of the docker-compose.yml file
services:    # Must be present exactly once at the beginning of the docker-compose.yml file
    relay-envelopes:
        image: chrberger/cluon-record:v0.0.3
        restart: always
        network_mode: "host"
        volumes:
        - $HOME/recordings:/recordings
        working_dir: /recordings
        command: "--cid=111"
```

Command for commandline to display the resulting image after operations:
```
docker run --rm -ti --init --net=host -v $HOME/recordings:/recordings -w /recordings chrberger/cluon-record:v0.0.3 --cid=111
```

The parameters to the application are:
* `--cid`: CID of the OD4Session to receive Envelopes for recording
* `--rec`: name of the recording file; default: YYYY-MM-DD_HHMMSS.rec
* `--recsuffix`: additional suffix to add to the .rec file
* `--remote`: listen for cluon.data.RecorderCommand to start/stop recording

## Build from sources on the example of Ubuntu 16.04 LTS
To build this software, you need cmake, C++14 or newer, libyuv, libvpx, and make.
Having these preconditions, just run `cmake` and `make` as follows:

```
mkdir build && cd build
cmake -D CMAKE_BUILD_TYPE=Release ..
make && make test && make install
```


## License

* This project is released under the terms of the GNU GPLv3 License

