/* Pointer typedefs for the safe layer's resource fields: `anvil ffi gen`
   makes an opaque type `pub` only when another generated module names it,
   and sqlite3.h has no pointer typedefs of its own. */
#include <sqlite3.h>

typedef sqlite3 *sqlite3_ptr;
typedef sqlite3_stmt *sqlite3_stmt_ptr;
