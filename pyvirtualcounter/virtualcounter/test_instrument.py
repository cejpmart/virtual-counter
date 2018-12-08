from .drawtable import Table
from .instrument import Instrument
import time

PORT = '/dev/ttyACM0'
BAUDRATE = 57600

def test_counting_mode_measurement():
    '''
    Required hardware setup for Nucleo-F303RE:
     - bridge pin A0 to D12
    '''

    instrument = Instrument.open_serial_port(port=PORT, baudrate=BAUDRATE, timeout=1)
    generator = instrument.get_pwm_channel(0)
    meas = instrument.get_frequency_measurement_function()

    min, max = meas.get_range()

    freq = 1
    steps = [2/1, 5/2, 10/5]

    t = Table([('expected [Hz]',    8,  '%d'),
               ('measured [Hz]',    8,  '%d'),
               ('error [%]',        6,  '%6.2f'),
               ('gate time [s]',    6,  '%6.3f'),
               ('duration [s]',     6,  '%6.3f')
               ])

    # loop over frequencies 1, 2, 5, 10, 20, ...
    while freq < max:
        generator.set_frequency(freq)

        gate_time = meas.suggest_gate_time(frequency=freq, required_error=0.01)
        start = time.time()
        measured_freq = meas.do_measurement(gate_time)
        end = time.time()

        error = abs(measured_freq - freq) / freq
        t.row(int(freq), int(measured_freq), error * 100, gate_time, end - start)

        freq = freq * steps[0]
        steps = steps[1:] + steps[:1]
