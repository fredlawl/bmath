vim.filetype.add({
	extension = {
		bmath = "bmath",
	},
})

vim.lsp.config("bmath", {
	-- Command and arguments to start the server.
	cmd = {
		"/home/fred/Projects/bmath/build/bmath-ls",
		"-debug",
		"-log",
		"nvim-lsp.log",
		"-playback-log",
		"playback.log",
	},
	--cmd_cwd = "",
	-- Filetypes to automatically attach to.
	filetypes = { "bmath" },
	-- Sets the "workspace" to the directory where any of these files is found.
	-- Files that share a root directory will reuse the LSP server connection.
	-- Nested lists indicate equal priority, see |vim.lsp.Config|.
	root_markers = { ".bmath", ".git" },
	-- Specific settings to send to the server. The schema is server-defined.
	-- Example: https://raw.githubusercontent.com/LuaLS/vscode-lua/master/setting/schema.json
	settings = {},
	trace = "verbose",
})

--vim.lsp.log.set_level("debug")
--vim.lsp.log.set_format_func(vim.inspect)
vim.lsp.enable("bmath")
