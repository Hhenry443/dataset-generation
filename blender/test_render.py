#----------IMPORTS----------#

import blenderproc as bproc
import numpy as np
import bpy
import matplotlib
import math
import os
import csv
import sys
import argparse
from PIL import Image
from mathutils import Matrix

#----------INITILISATION----------#

bproc.init()

#----------PARSER ARGUMENTS----------#

parser = argparse.ArgumentParser()

# Output directory for the generated images and the transformation data
parser.add_argument(
    "--output-dir",
    type=str,
    default="output",
    help="Directory where generated images will be saved"
)

# Seed Generation (for Reproducibility)
parser.add_argument(
    "--seed",
    type=int,
    default=0,
    help="Random seed for generation"
)

# Image Resolution
parser.add_argument(
    "--image-size",
    type=int,
    default=256,
    help="Resolution of the generated images (width and height)"
)

# Number of Images Generated (Per Process)
parser.add_argument(
    "--num-images",
    type=int,
    default=10,
    help="Number of images to generate"
)

# Distance of Camera to Object
parser.add_argument(
    "--camera-distance",
    type=float,
    default=2.5,
    help="Distance of the camera from the object"
)

args = parser.parse_args()

np.random.seed(args.seed)

output_dir = args.output_dir
os.makedirs(output_dir, exist_ok=True)

#----------LOADING DATA MODELS----------#

#Loads the object list used in the Equicaps dataset
objects = np.load("data/all_objects.npy", allow_pickle=True)

# Filter to Only Include Airplanes (for Testing)
plane_objects = objects[objects[:, 0] == "02691156"]

# Only Including Models that are Available Locally
available_planes = [str(obj[1]) for obj in plane_objects if os.path.isdir(os.path.join("data", "02691156", str(obj[1])))]

if not available_planes:
    raise RuntimeError("No matching airplane models found in the data directory.")

# Randomly select one airplane
selected_object = np.random.choice(available_planes)

print(f"Selected airplane: {selected_object}")

# Load the selected ShapeNet model
model = bproc.loader.load_shapenet("data", used_synset_id="02691156", used_source_id=selected_object)

# Move the model to the origin
bbox = model.get_bound_box()
centre = np.mean(bbox, axis=0)
model.set_location(-centre)

#----------SCENE SETUP----------#

# Large Plane Acting like the Scene Floor
bpy.ops.mesh.primitive_plane_add(size=10000, location = (0, 0, -1))
floor = bpy.context.active_object

# Material for the Floor
mat = bpy.data.materials.new(name="floorMaterial")
mat.use_nodes = True
floor.data.materials.append(mat)

# Get the Principled BSDF shader
bsdf = mat.node_tree.nodes.get("Principled BSDF")

#----------LIGHTING SETUP----------#

# Converting Spherical to Cartesian Coordinates
def spherical_to_cartesian(r, theta, phi):
    return np.array([
        r * np.sin(theta) * np.cos(phi),
        r * np.sin(theta) * np.sin(phi),
        r * np.cos(theta)
    ])

# Sun to Create General Scene Illumination
sun = bproc.types.Light()
sun.set_type("SUN")
sun.set_energy(1.5)
sun.blender_obj.data.angle = np.pi / 2 

# Spotlight to Provide Directional Lighting
light = bproc.types.Light()
light.set_type("SPOT")
light.set_location([0, 0, 2])
light.set_rotation_euler([0, 0, 0])
light.set_energy(500)
light.blender_obj.data.spot_size = np.pi / 8

#----------CAMERA SETUP----------#

image_size = args.image_size
distance = args.camera_distance

bproc.camera.set_resolution(image_size, image_size)

# Camera Position using Spherical Coordinated
camera_theta = np.pi / 4
location = spherical_to_cartesian(distance, camera_theta, np.pi / 2)

# Camera Toward the Origin
rotation_matrix = bproc.camera.rotation_from_forward_vec(np.array([0, 0, 0]) - location)

camera_pose = bproc.math.build_transformation_mat(location,rotation_matrix)
bproc.camera.add_camera_pose(camera_pose) #frame = 0?

#----------RENDERING SETTINGS----------#

# Enabling Distance Output alongside Colour Rendering
bproc.renderer.enable_distance_output(activate_antialiasing=False)

# Setting the Max Amount of Samples used for Colour Rendering
bproc.renderer.set_max_amount_of_samples(100)

generation_quantity = args.num_images

# Storing Transformation Data used for Each Generated Image.
transformation_data = []

#----------IMAGE GENERATION----------#

for i in range(generation_quantity):

    # Randomly Generated Rotational Angles.
    yaw = np.random.uniform(-math.pi / 2, math.pi / 2)
    pitch = np.random.uniform(-math.pi / 2, math.pi / 2)
    roll = np.random.uniform(-math.pi / 2, math.pi / 2)

    # Randomly Generated Object Translation
    x = np.random.uniform(-0.5, 0.5)
    y = np.random.uniform(-0.5, 0.5)
    z = np.random.uniform(-0.5, 0.5)

    model.set_rotation_euler([pitch, roll, yaw])

    translation = [x, y, z]
    model.set_location(translation)

    # Randomising the Floor Colour with a Randomly Generated Hue
    floor_hue = np.random.uniform(0, 1)
    hsv = (floor_hue, 0.6, 0.6)
    rgb = matplotlib.colors.hsv_to_rgb(hsv)

    # Update the Floor Material's Base Colour
    bsdf.inputs["Base Color"].default_value = (*rgb, 1)

    # Randomly Generated Spotlight Position
    spot_theta = np.random.uniform(0, np.pi / 4)
    spot_phi = np.random.uniform(0, 2 * np.pi)

    spot_location = spherical_to_cartesian(4, spot_theta, spot_phi)

    # Pointing the Spotlight Towards the Origin
    rotation_matrix = bproc.camera.rotation_from_forward_vec(np.array([0, 0, 0]) - spot_location)

    # Spotlight Transformation Matrix
    light_pose = Matrix(bproc.math.build_transformation_mat(spot_location, rotation_matrix))
    light.blender_obj.matrix_world = light_pose

    # Randomly Generated Spotlight Colours
    spot_hue = np.random.uniform(0, 1)
    hsv = (spot_hue, 1, 0.8)
    rgb = matplotlib.colors.hsv_to_rgb(hsv)
    light.set_color(rgb)

    # Recording Transformation Data for This Image
    transformation_data.append([i, yaw, pitch, roll, floor_hue, x, y, z, spot_theta, spot_phi, spot_hue])

    # Rendering the Scene
    data = bproc.renderer.render()

    # Save Image
    image = data["colors"][0]
    Image.fromarray(image).save(os.path.join(output_dir, f"image_{i}.png"))

    print(f"Generated image {i + 1}/", generation_quantity)

#----------SAVE TRANSFORMATION DATA----------#

# Saving the Transformation Parameters into a CSV File
with open(os.path.join(output_dir, "transformations.csv"), "w", newline = "") as file:
    writer = csv.writer(file)

    writer.writerow(["image", "yaw", "pitch", "roll", "floor_hue", "x", "y", "z", "spot_theta", "spot_phi", "spot_hue"])
    writer.writerows(transformation_data)

print("Completed with no errors.")
