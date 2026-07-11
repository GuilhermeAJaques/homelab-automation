import gpiod

class GPIO_driver:
    # Just declare to match with other drivers
    def __init__(self, chip):
        try:
            self.chip = gpiod.Chip(chip)
        except Exception as e:
            print("Error connectiong to gpiochip0: {}".format(e)) 

    def read_variable(self, gpio_number):
        try:
            line = self.chip.get_line(gpio_number)
            line.request(consumer="reader", type=gpiod.LINE_REQ_DIR_IN)
            value = line.get_value()
            line.release()
            return value
        except Exception as e:
            print("Error to read {}: {}".format(gpio_number, e)) 
            try:
                line.release()
            except:
                pass
            return None

    def write_variable(self, gpio_number, value):
        try:
            line = self.chip.get_line(gpio_number)
            line.request(consumer="writer", type=gpiod.LINE_REQ_DIR_OUT)
            line.set_value(value)
            line.release()
        except Exception as e:
            print("Error to write {}: {}".format(gpio_number, e)) 
            try:
                line.release()
            except:
                pass

    # Just declare to match with other drivers, otherwise will throw error
    def connect(self):
        print("GPIO driver connected")

    # Just declare to match with other drivers, otherwise will throw error
    def disconnect(self):
        try:
            self.chip.close()
        except Exception as e:
            print("Error close connection to GPIO: {}".format(e)) 
        print("GPIO driver disconnected")