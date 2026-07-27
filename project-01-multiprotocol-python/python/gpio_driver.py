import gpiod
from gpiod.line import Direction, Value
from Logger import Logger, Criticality, Class

class GPIO_driver:
    # Just declare to match with other drivers
    def __init__(self, chip, output_pins):
        try:
            self.chip = chip
            self.output_request = None

            # Get all output pins
            config = {}
            for pin in output_pins:
                config[pin] = gpiod.LineSettings(direction=Direction.OUTPUT)

            if config:
                self.output_request = gpiod.request_lines(
                    self.chip,
                    consumer="writer",
                    config=config
                )
        except Exception as e:
            Logger.log(Class.GPIO, Criticality.ERROR ,"Error connectiong to gpiochip0: {}".format(e)) 
        
    def read_variable(self, gpio_number):
        try:
            with gpiod.request_lines(self.chip, 
                                     consumer="reader",
                                     config={gpio_number: gpiod.LineSettings(direction=Direction.INPUT)}) as request:
                value = request.get_value(gpio_number)
                return value == Value.ACTIVE
        except Exception as e:
            Logger.log(Class.GPIO, Criticality.ERROR ,"Error to read {}: {}".format(gpio_number, e)) 
            return None

    def write_variable(self, gpio_number, value):
        try:
            if bool(value):
                self.output_request.set_value(gpio_number, Value.ACTIVE)
            else:
                self.output_request.set_value(gpio_number, Value.INACTIVE)
        except Exception as e:
            Logger.log(Class.GPIO, Criticality.ERROR ,"Error to write {}: {}".format(gpio_number, e)) 

    # Just declare to match with other drivers, otherwise will throw error
    def connect(self):
        Logger.log(Class.GPIO, Criticality.INFO ,"GPIO driver connected")

    # Just declare to match with other drivers, otherwise will throw error
    def disconnect(self):
        Logger.log(Class.GPIO, Criticality.INFO ,"GPIO driver disconnected")