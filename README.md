# sqlite

Ferron bindings to the system SQLite 3 (`libsqlite3`, any 3.x from 3.37):
the generated `extern "C"` module in `src/sys/` (package-internal) and a safe
layer over it with the API of the former `std.sqlite`, backed by the real
engine (indexes, joins, aggregates, WAL, everything SQLite has).

## Use

Add it to the `anvil.toml` of a package as a git dependency pinned by tag:

```toml
[dependencies]
sqlite = { git = "https://github.com/ferron-lang/sqlite", tag = "v0.3.0" }
```

and import it as `sqlite`. Programs using the package link the system
`libsqlite3` (`[native] link` in this package's `anvil.toml`), so
`cc -lsqlite3` must find it (`libsqlite3-dev` on Debian and Ubuntu,
`sqlite` on Arch, `sqlite-devel` on Fedora). Versions stay `v0.x` until the
API settles: the minor version marks changes that may break callers, the
patch version fixes; `CHANGELOG.md` lists them.

```ferron
use std.data.Value
use std.env.Env
use std.fs.Path
use sqlite.database.Database
use sqlite.error.SqliteError

#[derive(Data)]
struct User {
  id: Int,
  name: String,
  age: Option[Int],
}

#[derive(Data)]
struct Older {
  min: Int,
}

fn main(env: Env) -> Result[Unit, SqliteError] {
  let db = Database.open(env.fs, Path.from("app.db"))?
  let _ = db.execute("CREATE TABLE IF NOT EXISTS users (id INTEGER PRIMARY KEY, name TEXT, age INT)", Value.Null)?
  let _ = db.execute("INSERT INTO users (name, age) VALUES (?, ?)", ("ann", 31))?
  let users: List[User] = db.query("SELECT * FROM users WHERE age > :min", Older { min: 30 })?
  for user in users {
    env.console.print("{user.id} {user.name} {user.age}\n")
  }
  with tx = db.transaction()? {
    let _ = tx.execute("DELETE FROM users WHERE age IS NULL", Value.Null)?
    tx.commit()?
  }
  return Ok(Unit)
}
```

- `sqlite.database.Database`: a connection, closed with `sqlite3_close_v2`
  when its owner's scope ends (or by `close()`). `Database.open(env.fs,
  path)` opens a file, creating it when missing; `Database.memory()` is an
  in-memory database. `execute(sql, params)` runs every statement of `sql`
  and returns the number of rows they changed as an `Int`;
  `query(sql, params)` returns the rows of the last statement, each decoded
  from a `Value.Map` of column names through `FromData` (a blob arrives as
  base64 text). Parameters: a list or tuple binds `?` and `?N` by position,
  a struct or `Map[String, _]` binds `:name`, `@name`, and `$name`, a
  single scalar binds the one parameter, and `Value.Null` binds none;
  values are bound through `sqlite3_bind_*`, never spliced into the SQL.
- `sqlite.database.Transaction`: `db.transaction()` runs `BEGIN`; `commit()`
  runs `COMMIT`; closing it any other way (`rollback()`, the end of a `with`
  block, an early return, a failed commit) runs `ROLLBACK`. It uses the
  database's connection, so statements on the database while it is open
  belong to it.
- `sqlite.error.SqliteError`: `kind` (a `SqliteErrorKind` from the primary
  result code: `Sql`, `Constraint`, `Mismatch`, `Busy`, `ReadOnly`,
  `TooLarge`, `Corrupt`, `CantOpen`, `Io`, `Misuse`, `Other`, plus this
  package's `Parameter` and `Decode`), `code` (SQLite's extended result
  code), and `message` (`sqlite3_errmsg`). It displays as `<kind>:
  <message>`, such as `Constraint: UNIQUE constraint failed: users.name`.

Calls into SQLite are direct (FFI stage 1 has no blocking pool), so a long
query occupies its actor's worker thread.

### From `std.sqlite`

After adding the dependency, import `Database` and `Transaction` from
`sqlite.database` and `SqliteError` and `SqliteErrorKind` from
`sqlite.error` instead of `std.sqlite`. `Database`, `Transaction`,
`execute`, `query`, parameters, and row decoding keep their shape. What
changed:

- Error kinds follow SQLite's result codes: `Syntax`, `NoSuchTable`,
  `NoSuchColumn`, and `TableExists` are all `Sql` (the message tells them
  apart), the former `Unsupported` cases now work, and `SqliteError` gained
  `code`. `Io` carries no `IoErrorKind`.
- A `Transaction` runs on the database's own connection instead of a
  private copy: statements on the database while it is open are part of
  it, and a commit no longer fails with `Busy` because the database
  committed first.
- `Database.simulate_crash` is gone; SQLite's journal (or WAL) handles
  crash recovery.

## Develop

Needs the SQLite headers (`sqlite3.h` in `/usr/include`) and `clang` (for
`anvil ffi gen`).

```sh
anvil ffi gen --check
anvil check
anvil test          # every module's tests; `anvil test pkg.database` filters
anvil fmt
```

## C bindings

`anvil ffi gen` writes `src/sys/sqlite3.fer` from `sqlite3.h`. `ffi.toml`
pins the binding to the functions the safe layer calls (`ignore = "."` plus
a `require` list), so the output is the same for every SQLite 3.x header
from 3.37 on; using another function means adding it to `require`.
`sqlite3_bind_text` takes a destructor callback (a `CFn`); the package
always passes `SQLITE_TRANSIENT`, the address `-1`, built with
`CFn[Ptr[Unit], Unit].of(Ptr[Unit].from_address(SQLITE_TRANSIENT))`, so
SQLite copies the text.

## CI

`.github/workflows/ci.yml` builds the compiler from `ferron-lang/ferron` at
`FERRON_REF`, a release tag or a commit, installs `libsqlite3-dev` and
`clang`, and runs `anvil check`, `anvil test` (checking that the runtime
reports `live_handles=0` afterwards), `anvil fmt --check`, and
`anvil ffi gen --check`. `ferron-lang/ferron` is private, so the workflow
checks it out with the repository secret `FERRON_REPO_TOKEN`: a fine-grained
personal access token with read access to the contents of
`ferron-lang/ferron`.

## License

MIT; see `LICENSE`.
