# Cambia 'modelo_esp32.tflite' por la ruta a tu archivo
input_file = "modelo_esp32.tflite"
output_file = "modelo_esp32.h"

with open(input_file, "rb") as f:
    data = f.read()

# Convertir los datos binarios a formato hexadecimal
hex_array = [f"0x{byte:02X}," for byte in data]

# Formatear el resultado en filas más pequeñas para el archivo .h
chunk_size = 12  # Puedes ajustar el tamaño de los bloques de bytes
formatted_array = []
for i in range(0, len(hex_array), chunk_size):
    formatted_array.append("    " + " ".join(hex_array[i:i + chunk_size]))

# Crear el archivo .h con el array
with open(output_file, "w") as f:
    f.write("#ifndef MODELO_ESP32_H\n#define MODELO_ESP32_H\n\n")
    f.write(f"const unsigned char modelo_esp32[] = {{\n")
    f.write("\n".join(formatted_array))
    f.write(f"\n}};\n")
    f.write(f"const unsigned int modelo_esp32_len = {len(data)};\n\n")
    f.write("#endif // MODELO_ESP32_H\n")

print(f"Archivo convertido y guardado en {output_file}")
