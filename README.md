# BlockSci

This is a fork of the original BlockSci repository with extensions for the analysis of CoinJoin transactions.
This fork is a part of the [Master's thesis](https://is.muni.cz/th/qrvgf/Thesis.pdf) developed for CRoCS laboratory at Masaryk University, Brno, Czech Republic and use for [ESORICS'25 publication](https://crocs.fi.muni.cz/public/papers/coinjoins_esorics25).

## CLion development with Docker

Develop BlockSci in Docker rather than on the host. The project needs an older
toolchain (Ubuntu 20.04, Python 3.8.20, and GCC/G++ 7), configured by this
repository's `Dockerfile`. The host only needs Docker and CLion; CMake, the
compiler, and Python dependencies live in the dependency image, while `make`
and `gdb` are included in the CLion image.

### Prerequisites

1. Install Docker Engine and make sure the daemon is available:

   ```bash
   docker version
   ```

   If Docker requires root privileges on the host, prefix the Docker commands
   below with `sudo`. Alternatively, configure Docker's `docker` group according
   to Docker's host-installation documentation and start a new login session.
2. Clone this repository including submodules:

   ```bash
   git clone --recurse-submodules <repository-url> blocksci
   cd blocksci
   ```

   For an existing checkout, use `git submodule update --init --recursive`.

### Build the images

Run these commands from the repository root. Image builds download packages and
can take some time on their first run.

| Image tag | Docker target | Purpose |
| --- | --- | --- |
| `blocksci-dev:latest` | `dependencies` | Base development/dependency image; it contains no source copy or compiled BlockSci binary. |
| `blocksci-clion:latest` | `Dockerfile.clion` | CLion Docker toolchain based on `blocksci-dev:latest`, including `make` and `gdb`. |
| `blocksci-cj:latest` | `complete` | Self-contained, compiled BlockSci image. Source changes require rebuilding this image. |
| `blocksci-cj:test` | `test` | Validation image; its build also executes the C++ and Python test suites. |

For CLion, first build the base dependency image:

```bash
docker build --target dependencies -t blocksci-dev:latest .
```

The local `Dockerfile.clion` derives the CLion image used by this workspace
from that dependency image:

```bash
docker build -f Dockerfile.clion -t blocksci-clion:latest .
```

Build the complete image when a self-contained image is needed:

```bash
docker build --target complete -t blocksci-cj:latest .
```

Build the validation image only when intentionally running the test suites:

```bash
docker build --target test -t blocksci-cj:test .
```

Rebuild `blocksci-dev:latest` and then `blocksci-clion:latest` after changing
the `Dockerfile`, `pyproject.toml`, or `uv.lock`. The CLion image includes `make`
and `gdb` for building and debugging. Verify the CLion toolchain after a build with:

```bash
docker run --rm blocksci-clion:latest bash -lc '
  cmake --version
  make --version
  gcc-7 --version
  g++-7 --version
  gdb --version
'
```

### Configure the CLion Docker toolchain

1. Open the repository root in CLion.
2. Go to **Settings | Build, Execution, Deployment | Toolchains**, add a
   **Docker** toolchain, and select the Docker server and
   `blocksci-clion:latest` image. Keep CLion's default container settings
   (including `--rm`).
3. Let CLion detect CMake, make, the C/C++ compilers, and GDB. The detected
   defaults (`/usr/bin/cmake`, `make`, `cc`, `c++`, and GDB) are sufficient for
   the current image; do not override them unless you deliberately use another
   compiler. A GCC 7 configuration can be selected explicitly when needed:

   ```text
   C compiler:   /usr/bin/gcc-7
   C++ compiler: /usr/bin/g++-7
   Debugger:     /usr/bin/gdb
   CMake:        /usr/bin/cmake
   Build tool:   /usr/bin/make
   ```

4. Go to **Settings | Build, Execution, Deployment | CMake**, create or edit a
   profile, and select this Docker toolchain. Use a CLion-specific build
   directory such as `$PROJECT_DIR$/cmake-build-docker-debug`; do not use
   `build`, which is reserved for `build.sh`. The directory must be expressed
   as a host-project path (or the `$PROJECT_DIR$` macro), not as the
   container's mount path such as `/tmp/blocksci/cmake-build-debug`. CLion
   maps the host path into the Docker container automatically.
5. For a debug profile, use `Debug` as the build type and add this CMake
   option:

   ```text
   -DCMAKE_POLICY_VERSION_MINIMUM=3.5
   ```

   If CLion does not find the image's Python automatically, also add:

   ```text
   -DPYTHON_EXECUTABLE=/blocksci/.venv/bin/python3
   ```

CLion mounts the open project into the container while configuring and building,
so source edits remain on the host but compilation and debugging use the pinned
container environment.

#### CMake cache recovery

Do not reuse a build directory whose CMake cache was generated from a different
source path. This commonly surfaces while configuring the bundled GoogleTest
dependency as an error that its `CMakeCache.txt` was created under another
directory. Set a new host-visible build directory, for example
`$PROJECT_DIR$/cmake-build-docker-debug-fresh`, then apply the profile and
reload the CMake project.

If CMake reports that it wrote build files under `/tmp/blocksci/...` but CLion
then says it cannot read `CMakeCache.txt`, the profile was given the container
path. Replace it with `$PROJECT_DIR$/cmake-build-docker-debug` (or an absolute
path on the host) and reload. `/tmp/blocksci` is only the Docker toolchain's
internal mount point.

### Build from an interactive development shell

This is useful for verifying the same environment outside CLion. It is also the
correct way to run `build.sh`: the script deliberately uses `/mnt/blocksci`.

```bash
cd /path/to/blocksci
docker run --rm -it \
  -v "$PWD:/mnt/blocksci" \
  -w /mnt/blocksci \
  blocksci-dev:latest bash
```

Inside the container, run:

```bash
SKIP_JUPYTER=1 ./build.sh
```

The default uses one compilation thread. Increase it only if enough memory is
available, for example `SKIP_JUPYTER=1 NTHREADS=4 ./build.sh`.

Do not run `./build.sh` directly from the host shell: it will fail because the
script and its CMake cache use the container path `/mnt/blocksci`. If a cache
was created at another path, rerun the build from the mounted container shell.

`make install` runs as root in the container and files written to the mounted
`build/` directory can therefore be owned by root on the host. If needed after
an interactive build, restore ownership on the host with:

```bash
sudo chown -R "$USER:$USER" build
```

## Quickstart
This version of BlockSci is intended to be run in Docker. The Docker image pins Python 3.8.20 alongside many outdated libraries. If you **really** want to run it on your machine, you can try to follow the installation
instructions from the original repository, however, unless you are running Ubuntu 20.04, you will most likely encounter issues.

BlockSci is a high-performance tool for blockchain science and exploration. It consists of two main components: a C++ library that performs high-performance blockchain analysis, and a Python library to provide high-level access to the results.
For the Python library, which is the main interface for the user, we set up Jupyter notebooks with examples of how to use BlockSci for various analyses.

To run BlockSci in Docker, the following steps are required:

1. Clone this repository
    1. You also need to clone with submodules. For that, either use `--recurse-submodules` while cloning, or, in the cloned repository, follow with `git submodule init` and `git submodule update --recursive`.
2. Install [Docker](https://docs.docker.com/get-docker/)
3. Build the dependency image by running `docker build --target dependencies -t blocksci-dev:latest .` in the root of the repository. This only installs the toolchain and libraries; it does not compile BlockSci.
    1. We use `uv` to speed up the `blockscipy` installation, since it takes a long time.
    2. Use `NTHREADS=<number> ./build.sh` to control how many threads are used by the later BlockSci compilation.

Now - since BlockSci is a memory-intensive and disk-consuming tool, we **strongly** recommend you to not hold the BlockSci data directly in the image, but to mount a persistent volume to the container.

An example way to set up the mounts is as follows:

- in `/mnt/blocksci` we mount this repository to be able to develop and write code without rebuilding the image.
- in `/mnt/data` we mount the blockchain directory (e.g. `~/.bitcoin`). This folder is used just for the initial parsing and does not need to be a persistent volume, since it's just the full node folder.
- in `/mnt/anal` we mount the volume where the BlockSci data will be stored. This is the most important volume, as it will contain the parsed blockchain data and the results of the analyses. Be careful as this volume can grow quite large.

To run the container with the mounted volumes, you can use the following command:

```bash
docker run --rm --name blocksci_container -p <notebook port>:8888 -v <this repository folder>:/mnt/blocksci -v <bitcoin fullnode directory>:/mnt/data  -v <analysis volume>:/mnt/anal -it --entrypoint /bin/bash blocksci-dev:latest
```

The `--rm` flag removes the container when it exits. The `-p` flag is used to expose the Jupyter notebook on the specified port. The `-v` flag is used to mount the volumes. The `-it` flag is used to run the container in interactive mode. The `--entrypoint /bin/bash` flag is used to run the container in bash mode, so you can run the Jupyter notebook manually.

Now, as we have everything mounted and we are connected to the container, we can run the "classic" BlockSci setup.

1. First, `cd /mnt/blocksci`.
2. Run `./build.sh`
    1. This rebuilds the application with the correct development settings, correct filepaths etc.
    2. This also runs the Notebooks, so when they are started, just turn them off, as we have no parsed data yet.
2. Parse blockchain 
    1. `blocksci_parser <config file> generate-config bitcoin <blocksci data directory> --disk <fullnode data directory>`
        1. here, `<config file>` is a file that will be created, for example `/mnt/anal/blocksci_config.json`
        2. `<blocksci data directory>` is `/mnt/anal/blocksci_data` if you follow our example with the mounted volume 
        3. `<fullnode data directory>` is `/mnt/data`
    2. That just creates the configuration. To actually parse the blockchain, run `blocksci_parser <config file> update` 
    3. Now just wait for a while and everything should be parsed. We suggest for the **initial** run to have the blockchain directory on some fast read disk, as it takes a while.
3. After everything smoothly parses, run `./build.sh` once again. Now, there should be a Jupyter notebook running at port `<notebook port>`.
4. For smoother learning experience we suggest to read the docs below, as well as turning Hinterland on.
    1. [Hinterland](https://jupyter-contrib-nbextensions.readthedocs.io/en/latest/nbextensions/hinterland/README.html) is a jupyter extension that enables autocomplete in the notebook.

## Documentation from the original repository

We provide instructions in our [online documentation](https://citp.github.io/BlockSci/):

- [Installation instructions](https://citp.github.io/BlockSci/setup.html)
- [Using BlockSci](https://citp.github.io/BlockSci/using-blocksci.html)
- [Guide for the fluent interface](https://citp.github.io/BlockSci/fluent-interface.html)
- [Module reference for the Python interface](https://citp.github.io/BlockSci/reference/reference.html)
- [Troubleshooting](https://citp.github.io/BlockSci/troubleshooting.html)

Our [FAQ](https://github.com/citp/BlockSci/wiki) contains additional useful examples and tips.
