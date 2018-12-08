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

        # use recommended gate time for the desired precision
        gate_time = meas.suggest_gate_time(frequency=freq, desired_relative_error=0.01)

        # perform the measurement, and also take note of true measurement time
        # (to check that the gate timer is being configured correctly)
        start = time.time()
        measured_freq = meas.measure_frequency(gate_time)
        end = time.time()

        # calculate measurement error, display in table
        error = abs(measured_freq - freq) / freq
        t.row(int(freq), int(measured_freq), error * 100, gate_time, end - start)

        # for high frequencies, the PWM resolution limits what we can test without additional equipment
        if freq <= 2_000_000:
            assert error <= 0.01

        # generate next frequency in the series
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
               ('error [%]',        7,  '%7.3f'),
               ('periods measured', 6,  '%d'),
               ('duration [s]',     6,  '%6.3f')
               ])

    # loop over frequencies 1, 2, 5, 10, 20, ...
    while freq <= 10_000_000:
        generator.set_frequency(freq)

        num_periods = meas.suggest_num_periods(period=1 / freq, desired_relative_error=0.001)
        start = time.time()
        measured_period = meas.measure_period(num_periods)
        end = time.time()

        # calculate measurement error, display in table
        error = abs(measured_period - 1 / freq) * freq
        t.row(1 / freq, measured_period, error * 100, num_periods, end - start)

        # for high frequencies, the PWM resolution limits what we can test without additional equipment
        if freq <= 2_000_000:
            assert error <= 0.001

        # generate next frequency in the series
        freq = freq * steps[0]
        steps = steps[1:] + steps[:1]

#@pytest.mark.skip()
def test_phase_measurement():
    '''
    Required hardware setup for Nucleo-F303RE:
     - bridge pin A0 to D12
     - bridge pin A1 to D11
    '''

    instrument = Instrument.open_serial_port(port=PORT, baudrate=BAUDRATE, timeout=1)
    generator = instrument.get_pwm_channel(0)
    generator2 = instrument.get_pwm_channel(1)
    meas = instrument.get_phase_measurement_function()

    t = Table([('frequency [Hz]',   8,  '%d'),
               ('measured [Hz]',    8,  '%d'),
               ('phase [deg]',      6,  '%7.3f'),
               ('measured [deg]',   6,  '%7.3f'),
               ('error [deg]',      6,  '%7.3f'),
               ])

    for freq in [5, 1000, 20_000]:
        for phase_deg in range(0, 360, 60):
            generator.set_frequency(freq, phase_deg=0)
            generator2.set_frequency(freq, phase_deg=-phase_deg)

            measured_period, measured_phase = meas.measure_period_and_phase()

            # calculate measurement error, display in table
            error = abs(measured_phase - phase_deg)
            t.row(freq, round(1 / measured_period), phase_deg, measured_phase, error)

            assert error < 1

#@pytest.mark.skip()
def test_frequency_ratio_measurement():
    '''
    Required hardware setup for Nucleo-F303RE:
     - bridge pin A0 to D12
     - bridge pin D5 to D11
    '''

    instrument = Instrument.open_serial_port(port=PORT, baudrate=BAUDRATE, timeout=1)
    generator = instrument.get_pwm_channel(0)
    generator2 = instrument.get_pwm_channel(1)
    meas = instrument.get_frequency_ratio_measurement_function()

    t = Table([('frequency A [Hz]', 8,  '%d'),
               ('frequency B [Hz]', 8,  '%d'),
               ('ratio',            11, '%11.6f'),
               ('measured',         11, '%11.6f'),
               ('error [%]',        6,  '%6.2f'),
               ])

    for freq, freq2 in [(100, 10000), (10000, 10)]:
        generator.set_frequency(freq)
        generator2.set_frequency(freq2)

        ratio = freq / freq2
        measured_ratio = meas.measure_frequency_ratio(100)

        error = abs(measured_ratio - ratio) * ratio
        t.row(freq, freq2, ratio, measured_ratio, error * 100)
        assert error <= 0.01
