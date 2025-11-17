package bmath

import (
	"fred.software/m/pkg/lsp"
	"fred.software/m/pkg/util"
)

var Documentation = map[string]*lsp.SignatureInformation{
	"align": {
		Label:         "align(x, algin_to)",
		Documentation: util.Ptr("Aligns x to align_to."),
		Parameters: &[]lsp.ParameterInformation{
			{
				Label:         "x",
				Documentation: "Expression",
			},
			{
				Label:         "align_to",
				Documentation: "This should be a power of two, but not enforced.",
			},
		},
	},
	"align_down": {
		Label:         "align_down(x, align_to)",
		Documentation: util.Ptr("Aligns x to align_to, except the result is rounded down to nearest alignment."),
		Parameters: &[]lsp.ParameterInformation{
			{
				Label:         "x",
				Documentation: "Expression",
			},
			{
				Label:         "align_to",
				Documentation: "This should be a power of two, but not enforced.",
			},
		},
	},
	"bswap": {
		Label:         "bswap(x)",
		Documentation: util.Ptr("Swaps the byte order of x. 16, 32, and 64 swaps are implicit to number of bytes used in x."),
		Parameters: &[]lsp.ParameterInformation{
			{
				Label:         "x",
				Documentation: "Expression",
			},
		},
	},
	"clz": {
		Label:         "clz(x, num_bytes)",
		Documentation: util.Ptr("Counts the number of zeros before the first 0b1, according to num_bytes.\nExample:\nx = 1, num_bytes = 8, expect result 63. If num_bytes = 1, expect 7."),
		Parameters: &[]lsp.ParameterInformation{
			{
				Label:         "x",
				Documentation: "Expression",
			},
			{
				Label:         "num_bytes",
				Documentation: "Must be in range [1, 8].",
			},
		},
	},
	"ctz": {
		Label:         "ctz(x)",
		Documentation: util.Ptr("Counts number of zeros after the last 0b1.\nExample\nx = 1, expect result 0. If x = (1 << 63), expect 63."),
		Parameters: &[]lsp.ParameterInformation{
			{
				Label:         "x",
				Documentation: "Expression",
			},
		},
	},
	"mask": {
		Label:         "mask(num_bytes)",
		Documentation: util.Ptr("Creates a mask of 0xff according to num_bytes."),
		Parameters: &[]lsp.ParameterInformation{
			{
				Label:         "num_bytes",
				Documentation: "Must be in range of [0, 8]",
			},
		},
	},
	"popcnt": {
		Label:         "popcnt(x)",
		Documentation: util.Ptr("Counts the number of 0b1's set in x."),
		Parameters: &[]lsp.ParameterInformation{
			{
				Label:         "x",
				Documentation: "Expression",
			},
		},
	},
}
