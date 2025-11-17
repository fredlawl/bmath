## nvim client

Lazy:

_~/.config/nvim/lua/plugins/bmath.lua_
```
return {
  {
      dir = vim.fn.expand("~/Projects/bmath/bmath-client/bmath.nvim"),
      name = "bmath-lsp",
  },
}
```
