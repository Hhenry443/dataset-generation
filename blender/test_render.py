
import blenderproc as bproc
import numpy as np
import bpy
import matplotlib
import math
import os
import csv
import sys
import argparse

bproc.init()
np.random.seed(0) # "seed for reproducibility"


# Hopefully now compatable with multithreading if we can thread each instance with its own directory argument
parser = argparse.ArgumentParser()

parser.add_argument(
    "--output-dir",
    type=str,
    default="output",
    help="Directory where generated images will be saved"
)

args = parser.parse_args()

# The image will save to the specified output directory
output_dir = args.output_dir
os.makedirs(output_dir, exist_ok=True)

cube = bproc.object.create_primitive("CUBE")
cube.set_scale([1, 1, 1])

#Creating the floor mesh
bpy.ops.mesh.primitive_plane_add(size=10000, location = (0, 0, -1))
floor = bpy.context.active_object
#The material for the floor
mat = bpy.data.materials.new(name = "floorMaterial")
floor.data.materials.append(mat)


# Creating the Camera

# Creating the Camera
bproc.camera.set_resolution(256, 256)

# Position the camera (x, y, distance from cube z)
camera_location = np.array([3, -3, 10])

#Calculate the direction from the camera towards the origin
forward_vec = -camera_location

# Calculate the camera rotation so it points towards the object
camera_rotation = bproc.camera.rotation_from_forward_vec(forward_vec, "Z")

# Combine the camera position and rotation
camera_pose = bproc.math.build_transformation_mat(camera_location, camera_rotation)

bproc.camera.add_camera_pose(camera_pose)


#lighting
light = bproc.types.Light()
light.set_type("POINT")
light.set_location([3, -3, 5])
light.set_energy(1000)


generation_quantity = 10

#array to store the transformation data.
transformation_data = []

for i in range(generation_quantity):
    # Random rotation
    yaw = np.random.uniform(-math.pi / 2, math.pi / 2)
    pitch = np.random.uniform(-math.pi / 2, math.pi / 2)
    roll = np.random.uniform(-math.pi / 2, math.pi / 2)

    x = np.random.uniform(-0.5, 0.5)
    y = np.random.uniform(-0.5, 0.5)
    z = np.random.uniform(-0.5, 0.5)

    cube.set_rotation_euler([pitch, roll, yaw])

    # Random translations
    translation = [x, y, z]

    cube.set_location(translation)

    # Randomising the floor hue
    floor_hue = np.random.uniform(0, 1)

    # Converting HSV to RGB
    hsv = (floor_hue, 0.6, 0.6)
    rgb = matplotlib.colors.hsv_to_rgb(hsv)

    # Applying thee colour to floor
    floor.active_material.diffuse_color = (*rgb, 1)

    transformation_data.append([i, yaw, pitch, roll, floor_hue, x, y, z])

    # Rendering the Scene
    data = bproc.renderer.render()

    # Save image
    image = data["colors"][0]

    from PIL import Image
    Image.fromarray(image).save(
        os.path.join(output_dir, f"image_{i}.png")
    )

    print(f"Generated image {i + 1}/", generation_quantity)

with open(os.path.join(output_dir, "transformations.csv"), "w", newline = "") as file:
    writer = csv.writer(file)

    writer.writerow(["image", "yaw", "pitch", "roll", "floor_hue", "x", "y", "z"])

    writer.writerows(transformation_data)

print("Completed with no errors.")
#Run through terminal