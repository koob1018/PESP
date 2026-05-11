import unittest


FRAME_SOF0 = 0xA5
FRAME_SOF1 = 0x5A
CMD_READ_ALL = 0x11
CMD_READ_EVENT = 0x14
CMD_SET_FORCE_THRESHOLD = 0x21
CMD_BASE_BOOTSEL = 0x7E


def checksum(frame_type, payload):
    value = frame_type ^ len(payload)
    for byte in payload:
        value ^= byte
    return value


def encode_frame(frame_type, payload=b""):
    return bytes([FRAME_SOF0, FRAME_SOF1, frame_type, len(payload)]) + payload + bytes(
        [checksum(frame_type, payload)]
    )


def decode_frames(stream):
    frames = []
    state = "sof0"
    frame_type = 0
    length = 0
    payload = bytearray()
    crc = 0

    for byte in stream:
        if state == "sof0":
            if byte == FRAME_SOF0:
                state = "sof1"
        elif state == "sof1":
            state = "type" if byte == FRAME_SOF1 else "sof0"
        elif state == "type":
            frame_type = byte
            crc = byte
            state = "len"
        elif state == "len":
            length = byte
            payload.clear()
            crc ^= byte
            state = "crc" if length == 0 else "payload"
        elif state == "payload":
            payload.append(byte)
            crc ^= byte
            if len(payload) == length:
                state = "crc"
        elif state == "crc":
            if byte == crc:
                frames.append((frame_type, bytes(payload)))
            state = "sof0"

    return frames


class ProtocolFrameTest(unittest.TestCase):
    def test_checksum_matches_protocol_xor_definition(self):
        payload = b"1000"
        expected = CMD_SET_FORCE_THRESHOLD ^ len(payload)
        for byte in payload:
            expected ^= byte
        self.assertEqual(checksum(CMD_SET_FORCE_THRESHOLD, payload), expected)

    def test_empty_payload_round_trip(self):
        frame = encode_frame(CMD_READ_ALL)
        self.assertEqual(decode_frames(frame), [(CMD_READ_ALL, b"")])

    def test_ascii_payload_round_trip(self):
        frame = encode_frame(CMD_SET_FORCE_THRESHOLD, b"1000")
        self.assertEqual(decode_frames(frame), [(CMD_SET_FORCE_THRESHOLD, b"1000")])

    def test_response_frame_sets_high_bit(self):
        response_type = CMD_READ_ALL | 0x80
        payload = b"TEMP=27.77,HUM=15.43,PRESS=1021.46,LIGHT=4733,FORCE=31,EVENT=0"
        frame = encode_frame(response_type, payload)
        self.assertEqual(decode_frames(frame), [(response_type, payload)])

    def test_multiple_frames_in_one_stream(self):
        stream = encode_frame(CMD_READ_ALL) + encode_frame(CMD_READ_EVENT)
        self.assertEqual(
            decode_frames(stream),
            [(CMD_READ_ALL, b""), (CMD_READ_EVENT, b"")],
        )

    def test_bad_checksum_is_dropped(self):
        frame = bytearray(encode_frame(CMD_SET_FORCE_THRESHOLD, b"1000"))
        frame[-1] ^= 0x01
        self.assertEqual(decode_frames(frame), [])

    def test_noise_before_valid_frame_is_ignored(self):
        stream = b"noise\n" + encode_frame(CMD_READ_ALL)
        self.assertEqual(decode_frames(stream), [(CMD_READ_ALL, b"")])

    def test_valid_frame_after_bad_checksum_is_decoded(self):
        bad_frame = bytearray(encode_frame(CMD_SET_FORCE_THRESHOLD, b"1000"))
        bad_frame[-1] ^= 0x01
        stream = bytes(bad_frame) + encode_frame(CMD_READ_ALL)
        self.assertEqual(decode_frames(stream), [(CMD_READ_ALL, b"")])

    def test_incomplete_frame_is_ignored(self):
        frame = encode_frame(CMD_SET_FORCE_THRESHOLD, b"1000")
        self.assertEqual(decode_frames(frame[:-1]), [])

    def test_base_bootsel_maintenance_frame_round_trip(self):
        frame = encode_frame(CMD_BASE_BOOTSEL, b"BOOTSEL")
        self.assertEqual(decode_frames(frame), [(CMD_BASE_BOOTSEL, b"BOOTSEL")])


if __name__ == "__main__":
    unittest.main()
