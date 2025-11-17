package v2

import (
	"encoding/json"
)

type RequestID string

type Request struct {
	ID      *RequestID `json:"id,omitempty"`
	Version Version    `json:"jsonrpc"`
	Method  string     `json:"method"`
	Params  any        `json:"params,omitempty"`
}

func (r *Request) IsNotification() bool {
	return r.ID == nil
}

func (rid *RequestID) UnmarshalJSON(b []byte) error {
	var str string
	var num json.Number
	start := []byte("\"")
	for idx := range start {
		if b[idx] == start[idx] {
			err := json.Unmarshal(b, &str)
			if err != nil {
				return err
			}

			*rid = RequestID(str)
			return nil
		}
	}

	err := json.Unmarshal(b, &num)
	if err != nil {
		return err
	}

	*rid = RequestID(num.String())
	return nil
}

func (rid *RequestID) MarshalJSON() ([]byte, error) {
	bytes, err := json.Marshal(json.Number(*rid))
	if err != nil {
		return json.Marshal(string(*rid))
	}

	return bytes, err
}
