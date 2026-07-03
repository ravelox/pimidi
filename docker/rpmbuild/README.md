# Fedora RPM Build Validation

This stack validates the RPM packaging flow in a Fedora 44 container.

From the repository root:

```sh
docker compose -f docker/rpmbuild/compose.yaml up --build --abort-on-container-exit
```

The container runs:

```sh
./autogen.sh
./configure
make clean
./config.status raveloxmidi.spec
make -j$(nproc)
make dist
rpmbuild -ta raveloxmidi-*.tar.gz
```

RPM artifacts are written under the `rpmbuild-home` Docker volume at
`/home/builder/rpmbuild/RPMS` and `/home/builder/rpmbuild/SRPMS`, then
copied to `raveloxmidi/build/`.
