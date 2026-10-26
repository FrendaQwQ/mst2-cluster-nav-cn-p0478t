#!/usr/bin/env python3
"""Host checks of the actual C decoder with sanitizers, no phone or vendor code."""
import ctypes as C
from pathlib import Path
import struct
import subprocess
import unittest

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'build/check'
OUT.mkdir(parents=True, exist_ok=True)
LIB = OUT / 'route.dylib'
subprocess.run(['clang', '-shared', '-fPIC', '-DMCN_HOST', '-std=c99', '-Wall', '-Wextra',
                '-Werror', '-fsanitize=undefined', '-fno-sanitize-recover=all',
                '-I' + str(ROOT / 'include'), str(ROOT / 'src/common/route.c'),
                '-o', str(LIB)], check=True)
lib = C.CDLL(str(LIB))
lib.mcn_route_reset.argtypes = [C.c_void_p]
lib.mcn_route_log_reset.argtypes = [C.c_void_p]
lib.mcn_route_message.argtypes = [C.c_void_p, C.c_char_p, C.c_uint, C.c_uint]
lib.mcn_route_text.argtypes = [C.c_void_p, C.c_uint, C.c_void_p, C.c_uint]
lib.mcn_route_log_feed.argtypes = [C.c_void_p, C.c_void_p, C.c_char_p, C.c_uint, C.c_uint]
lib.mcn_route_config.argtypes = [C.c_char_p, C.c_uint, C.c_void_p, C.c_uint]
lib.mcn_route_component.argtypes = [C.c_void_p]
lib.mcn_route_start.argtypes = [C.c_void_p]


def tlv(key, data=b''):
    return struct.pack('>HH', len(data) + 4, key) + data


def num(key, value, size):
    return tlv(key, value.to_bytes(size, 'big', signed=value < 0))


def msg(kind, *fields):
    payload = b''.join(fields)
    return b'@@' + struct.pack('>HH', len(payload) + 6, kind) + payload


def params(data):
    result = {}
    while data:
        n, key = struct.unpack('>HH', data[:4])
        assert 4 <= n <= len(data)
        result[key] = data[4:n]
        data = data[n:]
    return result


class RouteCheck(unittest.TestCase):
    def setUp(self):
        self.route = C.create_string_buffer(65536)
        self.log = C.create_string_buffer(16384)
        lib.mcn_route_reset(self.route)
        lib.mcn_route_log_reset(self.log)

    def accept(self, packet, now=100):
        return lib.mcn_route_message(self.route, packet, len(packet), now)

    def snapshot(self, now=101):
        out = C.create_string_buffer(4096)
        n = lib.mcn_route_text(self.route, now, out, len(out))
        self.assertLess(n, len(out))
        return dict(line.split('=', 1) for line in out.value.decode().splitlines())

    def maneuver(self, index=7, *extra):
        return msg(0x5202, num(0, 16, 2), num(1, index, 2), num(3, 2, 1),
                   tlv(4, 'Седьмая\nactive=0'.encode()), num(5, 400, 4),
                   num(8, 1, 1), num(10, -90, 2), num(10, 90, 2), num(11, 90, 2), *extra)

    def status(self, now=100, index=7, *extra):
        return self.accept(msg(0x5201, num(1, 1, 1), num(10, 250, 4),
                               num(6, 900, 8), num(7, 12000, 4),
                               tlv(13, struct.pack('>H', index)), *extra), now)

    def test_actual_snapshot_and_cached_maneuver(self):
        self.assertEqual(self.accept(self.maneuver()), 1)
        self.assertEqual(self.status(), 1)
        s = self.snapshot()
        self.assertEqual([s[k] for k in ('active', 'type', 'angle', 'lht', 'dist', 'step',
                                        'rtt', 'dest', 'roads')],
                         ['1', '2', '90', '1', '250', '400', '900', '12000', '-90,90'])
        # Nothing passed yet and no current road: the next maneuver's road is not shown.
        self.assertEqual(s['street'], '')
        self.status(now=2000)
        self.assertEqual(self.snapshot(2001)['type'], '2')
        # Past maneuver 7 the car is on the road it led onto, line breaks removed.
        self.accept(msg(0x5201, num(10, 20, 4)), 2050)
        self.status(now=2100, index=8)
        self.assertEqual(self.snapshot(2101)['street'], 'Седьмая active=0')

    def test_empty_list_holds_then_hides_and_recovers_cache(self):
        self.accept(self.maneuver())
        self.status()
        self.accept(msg(0x5201, tlv(13)), 1000)
        self.assertEqual(self.snapshot(3999)['type'], '2')
        self.assertEqual(self.snapshot(4000)['type'], '-1')
        self.accept(msg(0x5201, tlv(13)), 4100)
        self.assertEqual(self.snapshot(4101)['type'], '-1')
        self.status(now=4200)
        self.assertEqual(self.snapshot(4201)['type'], '2')

    def test_silence_keeps_guidance(self):
        # The phone sends only changes: Yandex Maps, parked, every ~6 s.
        self.accept(self.maneuver())
        self.status()
        self.assertEqual(self.snapshot(60000)['type'], '2')

    def current(self, index, dist=None, *extra):
        if dist is not None:
            extra = (num(10, dist, 4),) + extra
        self.accept(msg(0x5201, num(1, 1, 1), tlv(13, struct.pack('>HH', index, 0xffff)), *extra))
        return self.snapshot()['street']

    def yandex(self, index, kind, road):
        self.accept(msg(0x5202, num(0, 16, 2), num(1, index, 2), num(3, kind, 1),
                        tlv(2, road.encode())))

    def test_street_is_the_road_the_car_is_on(self):
        # Yandex Maps (car, 2026-10-26): no current road, the road a maneuver leads onto only in
        # its description, and a new index on every update, the same maneuver included.
        self.yandex(19, 2, 'Первая ул.')
        self.assertEqual(self.current(19, 890), '')
        self.yandex(20, 2, 'Первая ул.')
        self.assertEqual(self.current(20, 240), '')
        self.yandex(21, 2, 'Первая ул.')
        self.assertEqual(self.current(21, 80), '')
        self.current(21, 10)
        # Turned: a different maneuver replaces it within 50 m.
        self.yandex(22, 14, 'Вторая пл.')
        self.assertEqual(self.current(22, 1400), 'Первая ул.')
        # A reroute (state 5) that replaces the maneuver near it is not a pass.
        self.current(22, 30)
        self.accept(msg(0x5201, num(1, 5, 1)))
        self.yandex(23, 47, 'Третья ул.')
        self.assertEqual(self.current(23, 100), 'Первая ул.')
        # A stop-and-restart reroute (state 0, 3, 1) keeps the road too.
        self.accept(msg(0x5201, num(1, 0, 1)))
        self.accept(msg(0x5201, num(1, 3, 1)))
        self.yandex(24, 2, 'Четвёртый пер.')
        self.assertEqual(self.current(24, 90), 'Первая ул.')
        # Past an unnamed turn the road is not known.
        self.current(24, 10)
        self.yandex(25, 2, '')
        self.assertEqual(self.current(25, 600), 'Четвёртый пер.')
        self.current(25, 10)
        self.yandex(26, 2, 'Вторая пл.')
        self.assertEqual(self.current(26, 970), '')
        # The app's current road wins when it sends one, and an empty one keeps it.
        self.assertEqual(self.current(26, None, tlv(3, 'Пятый б-р'.encode())),
                         'Пятый б-р')
        self.assertEqual(self.current(26, None, tlv(3, b'')), 'Пятый б-р')

    def test_instructions_are_never_a_road(self):
        # Apple Maps: roads in field 4, instructions in field 2 are never shown.
        self.accept(msg(0x5202, num(0, 16, 2), num(1, 1, 2), num(3, 2, 1),
                        tlv(4, 'ул. Шестая'.encode())))
        self.accept(msg(0x5202, num(0, 16, 2), num(1, 2, 2), num(3, 2, 1),
                        tlv(2, 'Поверните направо'.encode())))
        self.assertEqual(self.current(1, 300), '')
        self.current(1, 20)
        self.assertEqual(self.current(2, 500), 'ул. Шестая')
        self.accept(msg(0x5202, num(0, 16, 2), num(1, 3, 2), num(3, 2, 1)))
        self.current(2, 20)
        self.assertEqual(self.current(3, 400), '')

    def test_arrival_stop_timeout_and_new_route(self):
        self.accept(self.maneuver())
        self.status()
        self.accept(msg(0x5201, num(1, 2, 1)), 1000)
        self.assertEqual(self.snapshot(1001)['type'], '10')
        self.assertEqual(self.snapshot(6000), {'active': '0'})
        self.accept(msg(0x5201, num(1, 0, 1)), 6500)
        self.assertEqual(self.snapshot(6501), {'active': '0'})
        self.status(now=6600)
        self.assertEqual(self.snapshot(6601)['type'], '-1')

    def test_partial_fields_and_status_before_maneuver(self):
        self.status()
        self.assertEqual(self.snapshot()['type'], '-1')
        self.accept(self.maneuver())
        self.assertEqual(self.snapshot()['type'], '2')
        self.accept(msg(0x5202, num(1, 7, 2), num(3, 1, 1)))
        self.assertEqual(self.snapshot()['type'], '1')
        self.assertEqual(self.snapshot()['step'], '400')

    def test_malformed_and_foreign_component_do_not_mutate(self):
        self.accept(self.maneuver())
        self.status()
        before = bytes(self.route)
        for packet in (b'', b'@@\x00\x06R', msg(0x5201, b'\x00\x03\x00\x01'),
                       msg(0x5201, num(1, 0, 1), tlv(13, b'x')),
                       msg(0x5202, num(0, 17, 2), num(1, 7, 2), num(3, 1, 1)),
                       msg(0x5204, num(1, 1, 2), tlv(2, b'x'))):
            self.assertEqual(self.accept(packet), 0)
            self.assertEqual(bytes(self.route), before)

    def test_all_hex_read_splits_and_noise(self):
        packet = self.maneuver()
        data = ('--?-- [msgid=5202]\n' + ' '.join('%02X' % b for b in packet)
                + '\n(checksum) FF\n' + ' '.join('%02x' % b for b in
                  msg(0x5201, num(1, 1, 1), num(13, 7, 2))) + '\n').encode()
        for split in range(len(data) + 1):
            self.setUp()
            for part in (data[:split], data[split:]):
                lib.mcn_route_log_feed(self.log, self.route, part, len(part), 100)
            self.assertEqual(self.snapshot()['type'], '2', split)

    def test_stock_print_packet_framing(self):
        # ipod-drvr-iap2.so print_packet: "--?-- [msgid=%02x%02x]", " CTRL[" (or " CTRL data["),
        # ipod_print_bytes without a trailing space, "] ", "CKSUM[", one byte, "] ", ")".
        def dump(packet, label=' CTRL['):
            return ('--?-- [msgid=%02x%02x]' % (packet[4], packet[5]) + label
                    + ' '.join('%02X' % b for b in packet) + '] CKSUM[40] )\n')
        data = (dump(self.maneuver()) + dump(msg(0x5201, num(1, 1, 1), num(13, 7, 2)),
                                             ' CTRL data[')).encode()
        for split in range(len(data) + 1):
            self.setUp()
            count = 0
            for part in (data[:split], data[split:]):
                count += lib.mcn_route_log_feed(self.log, self.route, part, len(part), 100)
            self.assertEqual(count, 2, split)
            self.assertEqual(self.snapshot()['type'], '2', split)

    def test_oversize_and_broken_dump_recover(self):
        data = b'40 40 FF FF 52 02\n' + b'A' * 10000 + b' --broken--\n'
        data += b' '.join(('%02X' % b).encode() for b in self.maneuver()) + b'\n'
        lib.mcn_route_log_feed(self.log, self.route, data, len(data), 100)
        self.status()
        self.assertEqual(self.snapshot()['type'], '2')

    def test_reidentification_name_resets_cache_at_every_split(self):
        data = b'Accessory Identification StartIdentification (1d00)\n'
        for split in range(len(data) + 1):
            self.setUp()
            self.accept(self.maneuver())
            self.status()
            events = 0
            for part in (data[:split], data[split:]):
                events |= lib.mcn_route_log_feed(self.log, self.route, part, len(part), 200)
            self.assertTrue(events & 0x10000)
            self.assertEqual(self.snapshot(201), {'active': '0'})
            self.status(now=300)
            self.assertEqual(self.snapshot(301)['type'], '-1')

    def test_lanes_and_saturating_numbers(self):
        lanes = msg(0x5204, num(0, 16, 2), num(1, 5, 2),
                    tlv(2, num(0, 0, 2) + num(1, 2, 1) + num(2, -45, 2)))
        self.assertEqual(self.accept(lanes), 1)
        self.status(100, 7, num(16, 5, 2), num(18, 1, 1))
        self.accept(msg(0x5201, num(6, 0xffffffffffffffff, 8), num(7, 0xffffffff, 4)))
        self.assertEqual(self.snapshot()['rtt'], '2147483647')
        self.assertEqual(self.snapshot()['dest'], '2147483647')

    def lane(self, position, status, *angles, highlight=None):
        return tlv(2, num(0, position, 2) + num(1, status, 1)
                   + b''.join(num(2, a, 2) for a in angles)
                   + (num(3, highlight, 2) if highlight is not None else b''))

    def test_lane_snapshot_for_hmi(self):
        self.accept(msg(0x5204, num(1, 5, 2), self.lane(2, 2, 0, highlight=90),
                        self.lane(0, 0, -90), self.lane(1, 1, 0)))
        self.status(100, 7, num(16, 5, 2), num(18, 1, 1))
        out = C.create_string_buffer(4096)
        n = lib.mcn_route_text(self.route, 101, out, len(out))
        self.assertGreater(n, 0)
        self.assertIn(b'lane0=2,2,90,0\n', out.raw[:n])
        (OUT / 'lanes-message.txt').write_bytes(out.raw[:n])

    def test_lane_visibility_current_index_lifecycle_and_repeated_angles(self):
        self.accept(msg(0x5204, num(1, 5, 2), self.lane(0, 2, -90, 0, highlight=90)))
        self.status(100, 7, num(16, 5, 2), num(18, 1, 1))
        self.assertEqual(self.snapshot()['lanes'], '1')
        self.assertEqual(self.snapshot()['lane0'], '0,2,90,-90,0')
        self.accept(msg(0x5201, tlv(13)), 1000)
        self.assertEqual(self.snapshot(4001)['lanes'], '1')
        self.accept(msg(0x5201, num(18, 0, 1)), 4100)
        self.assertEqual(self.snapshot(4101)['lanes'], '0')
        self.accept(msg(0x5201, num(18, 1, 1), num(16, 6, 2)), 4200)
        self.assertEqual(self.snapshot(4201)['lanes'], '0')
        self.accept(msg(0x5204, num(1, 6, 2), self.lane(0, 1, 0)), 4300)
        self.assertEqual(self.snapshot(4301)['lane0'], '0,1,1000,0')
        self.accept(msg(0x5201, num(1, 2, 1)), 4400)
        self.assertEqual(self.snapshot(4401)['lanes'], '0')
        self.assertEqual(self.snapshot(9400), {'active': '0'})
        self.accept(msg(0x5201, num(1, 0, 1)), 9500)
        self.status(9600, 7, num(16, 6, 2), num(18, 1, 1))
        self.assertEqual(self.snapshot(9601)['lanes'], '0')

    def test_lane_capacity_and_output_bounds(self):
        packet = msg(0x5204, num(1, 5, 2), *[self.lane(i, 2, *range(-8, 8)) for i in range(16)])
        self.accept(packet)
        self.status(100, 7, num(16, 5, 2), num(18, 1, 1))
        self.assertEqual(self.snapshot()['lanes'], '16')
        small = C.create_string_buffer(128)
        self.assertLess(lib.mcn_route_text(self.route, 101, small, len(small)), 0)
        self.accept(msg(0x5204, num(1, 5, 2), *[self.lane(i, 2, 0) for i in range(17)]))
        self.assertEqual(self.snapshot()['lanes'], '0')
        self.accept(msg(0x5204, num(1, 5, 2), self.lane(0, 2, *range(17))))
        self.assertEqual(self.snapshot()['lanes'], '0')

    def test_group_and_start_wire_format(self):
        buf = C.create_string_buffer(128)
        n = lib.mcn_route_component(buf)
        fields = params(buf.raw[:n])
        self.assertEqual(fields[0], b'\x00\x10')
        self.assertEqual(fields[1], b'Instrument Cluster\0')
        self.assertEqual(fields[6], b'\x00\x06')
        self.assertEqual(fields[8], b'\x00\x06')
        n = lib.mcn_route_start(buf)
        self.assertEqual(buf.raw[:n], msg(0x5200, num(0, 16, 2)))

    def test_cfg_keeps_stock_values_and_is_idempotent(self):
        for data in (b'[accessory]\nserial=123\n',
                     b'[ident]\nsendmsgs=0x5000,20992\nrecvmsgs=0x5001\n[location]\nenable=1',
                     b'[ident]\nsendmsgs=\nrecvmsgs=\n',
                     b'[ident]\nsendmsgs=0x5200,0x5203\nrecvmsgs=0x5201,0x5202,0x5204\n'):
            out = C.create_string_buffer(4096)
            n = lib.mcn_route_config(data, len(data), out, len(out))
            self.assertGreater(n, 0)
            first = out.raw[:n]
            n = lib.mcn_route_config(first, len(first), out, len(out))
            self.assertEqual(out.raw[:n], first)
            self.assertEqual(first.count(b'sendmsgs='), 1)
            self.assertEqual(first.count(b'recvmsgs='), 1)
            if b'location' in data:
                self.assertTrue(first.endswith(b'[location]\nenable=1\n'))
                self.assertIn(b'sendmsgs=0x5000,20992,0x5203\n', first)
        self.assertEqual(lib.mcn_route_config(b'', 0, out, 4), -1)


if __name__ == '__main__':
    unittest.main(verbosity=2)
