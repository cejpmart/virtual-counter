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

    # loop over frequencies 1, 2, 5, 10, 20, ...
    while freq < max:
        generator.set_frequency(freq)

        gate_time = meas.suggest_gate_time(frequency=freq, required_error=0.01)
        start = time.time()
        measured_freq = meas.do_measurement(gate_time)
        end = time.time()

        error = abs(measured_freq - freq) / freq
        print(f'expected: {int(freq):8} Hz\t' +
              f'measured: {int(measured_freq):8} Hz\t' +
              f'error: {int(error*100):3} %\t' +
              f'gate time: {gate_time:5.2f} s\t' +
              f'duration: {end - start:6.3f} s')

        freq = freq * steps[0]
        steps = steps[1:] + steps[:1]
