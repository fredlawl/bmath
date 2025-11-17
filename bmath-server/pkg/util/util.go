package util

// Ptr takes a literal and converts it to a pointer.
func Ptr[T any](v T) *T {
	return &v
}

type IterResult[T any] struct {
	Item  *T
	Error error
}

func (ir *IterResult[T]) HasError() bool {
	return ir.Error != nil
}

func (ir *IterResult[T]) Dubious() bool {
	return ir.Item == nil
}
