from .drawtable import Table
from .instrument import Instrument
import pytest
import time

PORT = '/dev/ttyACM0'
BAUDRATE = 57600

#@pytest.mark.skip()
def test_frequency_measurement():
    '''
    Required hardware setup for Nucleo-F303RE:
     - bridge pin A0 to D12
    '''

    instrument = Instrument.open_serial_port(port=PORT, baudrate=BAUDRATE, timeout=1)
    generator = instrument.get_pwm_channel(0)
    meas = instrument.get_frequency_measurement_function()

    min, max = meas.get_frequency_range()

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

        gate_time = meas.suggest_gate_time(frequency=freq, desired_relative_error=0.01)
        start = time.time()
        measured_freq = meas.measure_frequency(gate_time)
        end = time.time()

        error = abs(measured_freq - freq) / freq
        t.row(int(freq), int(measured_freq), error * 100, gate_time, end - start)

        freq = freq * steps[0]
        steps = steps[1:] + steps[:1]

#@pytest.mark.skip()
def test_period_measurement():
    '''
    Required hardware setup for Nucleo-F303RE:
     - bridge pin A0 to D12
    '''

    instrument = Instrument.open_serial_port(port=PORT, baudrate=BAUDRATE, timeout=1)
    generator = instrument.get_pwm_channel(0)
    meas = instrument.get_period_measurement_function()

    _, max_freq = meas.get_frequency_range()

    freq = 0.1
    steps = [2/1, 5/2, 10/5]

    t = Table([('expected [s]',     9,  '%.7f'),
               ('measured [s]' ,    9,  '%.7f'),
               ('error [%]',        6,  '%6.2f'),
               ('periods measured', 6,  '%d'),
               ('duration [s]',     6,  '%6.3f')
               ])

    # loop over frequencies 1, 2, 5, 10, 20, ...
    while freq <= 10000000:
        generator.set_frequency(freq)

        num_periods = meas.suggest_num_periods(period=1 / freq, desired_relative_error=0.01)
        start = time.time()
        measured_period = meas.measure_period(num_periods)
        end = time.time()

        error = abs(measured_period - 1 / freq) * freq
        t.row(1 / freq, measured_period, error * 100, num_periods, end - start)

        freq = freq * steps[0]
        steps = steps[1:] + steps[:1]
