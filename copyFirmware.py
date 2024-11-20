import shutil
import os

def copy_firmware():
    # Define the source and destination paths
    source = os.path.join(".pio", "build", "esp32dev", "firmware.bin")
    destination = os.path.join("firmware", "firmware.bin")  # Your target folder

    # Copy the .bin file
    shutil.copy(source, destination)
    print(f"Firmware copied to {destination}")

# Run the copy function
copy_firmware()
