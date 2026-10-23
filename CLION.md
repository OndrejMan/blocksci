# CLion development with Docker

Develop BlockSci in Docker rather than on the host. The project needs an older
toolchain (Ubuntu 20.04, Python 3.8.20, and GCC/G++ 7), configured by this
repository's `Dockerfile`. The host only needs Docker and CLion; CMake, the
compiler, and Python dependencies live in the dependency image, while `make`
and `gdb` are included in the CLion image.

## Prerequisites

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

## Build the images

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

Rebuilds after a source change reuse compiler output from a BuildKit cache
mount (ccache), so only the changed sources compile again. The cache lives in
the builder, not in the image; `docker builder prune` clears it. With Docker 29,
`docker build --no-cache` was also observed to start from an empty cache.

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

## Configure the CLion Docker toolchain

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

### CMake cache recovery

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

## Build from an interactive development shell

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
