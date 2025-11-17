package v2

type Version string

func RpcVersion() Version {
	return Version("2.0")
}
