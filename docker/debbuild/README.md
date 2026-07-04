# Ubuntu Debian Package Build Validation

This stack validates the `.deb` packaging flow in an Ubuntu 24.04 container.

From the repository root:

```sh
docker compose -f docker/debbuild/compose.yaml up --build --abort-on-container-exit
```

The container runs:

```sh
./autogen.sh
./configure
make clean
make -j$(nproc)
make deb
pkgscripts/validate_deb_contents build/*.deb
```

Debian package artifacts are written to `raveloxmidi/build/`.
