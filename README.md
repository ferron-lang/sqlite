# sqlite

Ferron bindings to the system SQLite 3 (`libsqlite3`, any 3.x from 3.37):
the generated `extern "C"` module in `src/sys/` (package-internal) and a safe
layer over it.

## Use

Add it to the `anvil.toml` of a package as a git dependency pinned by tag:

```toml
[dependencies]
sqlite = { git = "https://github.com/ferron-lang/sqlite", tag = "v0.1.0" }
```

and import it as `sqlite`. Programs using the package link the system
`libsqlite3` (`[native] link` in this package's `anvil.toml`), so
`cc -lsqlite3` must find it (`libsqlite3-dev` on Debian and Ubuntu,
`sqlite` on Arch, `sqlite-devel` on Fedora).

## Develop

```sh
anvil check
anvil test
anvil fmt
```

## C bindings

`anvil ffi gen` writes the `extern "C"` modules in `src/sys/` from the
headers listed in `ffi.toml`; CI checks them with `anvil ffi gen --check`.
The C libraries to link go in `[native] link` of `anvil.toml`.

## CI

`.github/workflows/ci.yml` builds the compiler from `ferron-lang/ferron` at
`FERRON_REF`, a release tag or a commit, installs `libsqlite3-dev` and
`clang`, and runs `anvil check`, `anvil test`, `anvil fmt --check`, and
`anvil ffi gen --check`. `ferron-lang/ferron` is private, so the workflow
checks it out with the repository secret `FERRON_REPO_TOKEN`: a fine-grained
personal access token with read access to the contents of
`ferron-lang/ferron`.

## License

MIT; see `LICENSE`.
