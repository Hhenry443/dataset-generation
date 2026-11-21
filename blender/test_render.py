#
# Current Issues:
# Temporary cube sometimes appears partially clipped into the floor
# This could be solved once the true model imports are used.
#
# Installing the data from hugging face.
# I need an adviser.
# 
#




import blenderproc as bproc
import numpy as np
import bpy
import matplotlib
import math
import os
import csv
import sys
import argparse
from mathutils import Matrix #its fine, blenderproc provides its own mathutils library

bproc.init()

# Hopefully now compatible with multithreading if we can thread each instance with its own directory argument
parser = argparse.ArgumentParser()

parser.add_argument(
    "--output-dir",
    type=str,
    default="output",
    help="Directory where generated images will be saved"
)

# Changed the seed to be a parse argument so each thread generates its own saved seed
parser.add_argument(
    "--seed",
    type=int,
    default=0,
    help="Random seed for generation"
)

# argement to change the size of the image
parser.add_argument(
    "--image-size",
    type=int,
    default=256,
    help="Resolution of the generated images (width and height)"
)

# Argument to change the number of images generated.
parser.add_argument(
    "--num-images",
    type=int,
    default=10,
    help="Number of images to generate"
)

# Argument to change how far away the camera is from the object
parser.add_argument(
    "--camera-distance",
    type=float,
    default=2.5,
    help="Distance of the camera from the object"
)

# Argument for image count offset
parser.add_argument(
    "--num-offset",
    type=int,
    default=0,
    help="Offset for file names when outputting to one directory across processes"
)

args = parser.parse_args()

np.random.seed(args.seed) # "seed for reproducibility"

# The image will save to the specified output directory
output_dir = args.output_dir
os.makedirs(output_dir, exist_ok=True)

cube = bproc.object.create_primitive("CUBE")
cube.set_scale([1, 1, 1])

#Creating the floor mesh
bpy.ops.mesh.primitive_plane_add(size=10000, location = (0, 0, -1))
floor = bpy.context.active_object


#The material for the floor
#Fixed an issue with the floor material not actually changing the colour shader for rendering.
mat = bpy.data.materials.new(name="floorMaterial")
mat.use_nodes = True
floor.data.materials.append(mat)

# Get the Principled BSDF shader
bsdf = mat.node_tree.nodes.get("Principled BSDF")

#same as in Aiden's git
def spherical_to_cartesian(r, theta, phi):
    return np.array([
        r * np.sin(theta) * np.cos(phi),
        r * np.sin(theta) * np.sin(phi),
        r * np.cos(theta)
    ])

# Camera settings
image_size = args.image_size
distance = args.camera_distance #Aiden has his at 2.5 which is far too close for a cube test

bproc.camera.set_resolution(image_size, image_size)

# Camera position
camera_theta = np.pi / 4
location = spherical_to_cartesian(distance, camera_theta, np.pi / 2)

# Make the camera point towards the origin
rotation_matrix = bproc.camera.rotation_from_forward_vec(np.array([0, 0, 0]) - location)
camera_pose = bproc.math.build_transformation_mat(location,rotation_matrix)
bproc.camera.add_camera_pose(camera_pose) #frame = 0?

# Focal length is given in millimieters by default, so we convert it to meters
focal_length = bpy.data.cameras[0].lens/1000
print(f"focal length: {focal_length}m")

# Sun (pointed so that it casts no shadows on the floor like in Aiden's breif)
sun = bproc.types.Light()
sun.set_type("SUN")
sun.set_energy(1.5)
sun.blender_obj.data.angle = np.pi / 2 

'''
# Spot (main lighting)
light = bproc.types.Light()
light.set_type("SPOT")
light.set_location([0, 0, 2])
light.set_energy(500)
light.blender_obj.data.spot_size = np.pi / 2 # / 8 was the original but that was far too small
'''

# TEST: Spotlight directly above the cube
light = bproc.types.Light()
light.set_type("SPOT")
light.set_location([0, 0, 2])
light.set_rotation_euler([0, 0, 0])
light.set_energy(500)
light.blender_obj.data.spot_size = np.pi / 2 # / 8 was the original but that was far too small

#
# Personally from the tests the spotlight should be stronger and bigger than in the brief
# Maybe the sun also needs to be just a little dimmer
#


# activate normal and distance rendering
# if activate_antialiasing is True, uses Mist pass, else uses Z pass 
bproc.renderer.enable_distance_output(activate_antialiasing=False)
# set the amount of samples, which should be used for the color rendering
bproc.renderer.set_max_amount_of_samples(100)

generation_quantity = args.num_images

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
    #Same bug fix with the floor shaders
    floor_hue = np.random.uniform(0, 1)
    hsv = (floor_hue, 0.6, 0.6)
    rgb = matplotlib.colors.hsv_to_rgb(hsv)
    # Apply the colour to the floor material
    #This is changed from the original code which is floor.active_material.diffuse_color, because someone on stack overflow says this is a better solution
    bsdf.inputs["Base Color"].default_value = (*rgb, 1)

    # Random spotlight position
    spot_theta = np.random.uniform(0, np.pi / 4)
    spot_phi = np.random.uniform(0, 2 * np.pi)

    spot_location = spherical_to_cartesian(4, spot_theta, spot_phi)

    #pointing to the origin but using a matrix
    rotation_matrix = bproc.camera.rotation_from_forward_vec(np.array([0, 0, 0]) - spot_location)

    # Point the spotlight towards the origin
    # Convert to Blender's Matrix type, matching Aiden's original code
    light_pose = Matrix(bproc.math.build_transformation_mat(spot_location, rotation_matrix))
    # Testing fixing the spotlight to be directly above the cube
    light.blender_obj.matrix_world = light_pose

    #Random spotlight colours
    spot_hue = np.random.uniform(0, 1)
    hsv = (spot_hue, 1, 0.8)
    rgb = matplotlib.colors.hsv_to_rgb(hsv)
    light.set_color(rgb)



    #Recording the Data
    transformation_data.append([i + (generation_quantity * args.num_offset) + 1, yaw, pitch, roll, floor_hue, x, y, z, spot_theta, spot_phi, spot_hue])

    # Rendering the Scene
    data = bproc.renderer.render()

    # Save image
    image = data["colors"][0]

    from PIL import Image
    Image.fromarray(image).save(
        os.path.join(output_dir, f"image_{i + (generation_quantity * args.num_offset) + 1}.png") # Need to offset the file name as we put them all in one directory
    )

    print(f"Generated image {i + 1}/", generation_quantity)

with open(os.path.join(output_dir, "transformations.csv"), "a", newline = "") as file:
    writer = csv.writer(file)

    writer.writerow(["image", "yaw", "pitch", "roll", "floor_hue", "x", "y", "z", "spot_theta", "spot_phi", "spot_hue"])

    writer.writerows(transformation_data)

print("Completed with no errors.")
#Run through terminal

#blenderproc run blender/test_render.py \ --output-dir NewDirectory \ --seed NewValue \ --image-size NewValue \ --num-images NewValue \ --camera-distance NewValue
