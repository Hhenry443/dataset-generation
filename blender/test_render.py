#----------IMPORTS----------#

import blenderproc as bproc
import numpy as np
import bpy
import matplotlib
import math
import os
import csv
import sys
import json
import argparse
from PIL import Image
from mathutils import Matrix, Euler

#----------PARSER ARGUMENTS----------#

parser = argparse.ArgumentParser()

# Output directory for the generated images and the transformation data
parser.add_argument("--output-dir", type=str, default="output", help="Directory where generated images will be saved")

# Seed Generation (for Reproducibility)
parser.add_argument("--seed", type=int, default=0, help="Random seed for generation")

# Image Resolution
parser.add_argument("--image-size", type=int, default=256, help="Resolution of the generated images (width and height)")

# Number of Images Generated (Per Process)
parser.add_argument("--num-images", type=int, default=10, help="Number of images to generate")

# Distance of Camera to Object
parser.add_argument("--camera-distance", type=float, default=2.5, help="Distance of the camera from the object")

#----------ROTATION ARGEMENT RANGES----------#

# Yaw Rotation Range
parser.add_argument("--yaw-min", type=float, default=-math.pi / 2, help="Minimum yaw rotation in radians")
parser.add_argument("--yaw-max", type=float, default=math.pi / 2, help="Maximum yaw rotation in radians")

# Pitch Rotation Range
parser.add_argument("--pitch-min", type=float, default=-math.pi / 2, help="Minimum pitch rotation in radians")
parser.add_argument("--pitch-max", type=float, default=math.pi / 2, help="Maximum pitch rotation in radians")

# Roll rotation range
parser.add_argument("--roll-min", type=float, default=-math.pi / 2, help="Minimum roll rotation in radians")
parser.add_argument("--roll-max", type=float, default=math.pi / 2, help="Maximum roll rotation in radians")

#----------TRANSLATION ARGUMENT RANGES----------#

# X translation range
parser.add_argument("--x-min", type=float, default=-0.5, help="Minimum X translation")
parser.add_argument("--x-max", type=float, default=0.5, help="Maximum X translation")

# Y translation range
parser.add_argument("--y-min", type=float, default=-0.5, help="Minimum Y translation")
parser.add_argument("--y-max", type=float, default=0.5, help="Maximum Y translation")

# Z translation range
parser.add_argument("--z-min", type=float, default=-0.5, help="Minimum Z translation")
parser.add_argument("--z-max", type=float, default=0.5, help="Maximum Z translation")

#----------LIGHTING ARGUMENT RANGES----------#

# Floor colour hue range
parser.add_argument("--floor-hue-min", type=float, default=0.0, help="Minimum floor colour hue")
parser.add_argument("--floor-hue-max", type=float, default=1.0, help="Maximum floor colour hue")

# Spotlight position ranges
parser.add_argument("--spot-theta-min", type=float, default=0.0, help="Minimum spotlight theta in radians")
parser.add_argument("--spot-theta-max", type=float, default=np.pi / 4, help="Maximum spotlight theta in radians")

parser.add_argument("--spot-phi-min", type=float, default=0.0, help="Minimum spotlight phi in radians")
parser.add_argument("--spot-phi-max", type=float, default=2 * np.pi, help="Maximum spotlight phi in radians")

# Spotlight colour hue range
parser.add_argument("--spot-hue-min", type=float, default=0.0, help="Minimum spotlight colour hue")
parser.add_argument("--spot-hue-max", type=float, default=1.0, help="Maximum spotlight colour hue")

#----------MODEL ARGUMENT SELECTION----------#

# ShapeNet category ID
parser.add_argument("--model-category", type=str, default="02691156", help="ShapeNet category ID of the model, default ID is for Airplanes")
# ShapeNet model ID
parser.add_argument("--model-id", type=str, default=None, help="Specific ShapeNet model ID to load")

args = parser.parse_args()

#----------INPUT ARGUMENT VALIDATION----------#

# Checking that Image Generation Values are all Positive
if args.num_images <= 0:
    parser.error("--num-images must be greater than 0")

if args.image_size <= 0:
    parser.error("--image-size must be greater than 0")

if args.camera_distance <= 0:
    parser.error("--camera-distance must be greater than 0")

# Checking that all the Max Values are Equal or Greater than the Minimum Values.
ranges = [
    ("yaw", args.yaw_min, args.yaw_max),
    ("pitch", args.pitch_min, args.pitch_max),
    ("roll", args.roll_min, args.roll_max),
    ("x", args.x_min, args.x_max),
    ("y", args.y_min, args.y_max),
    ("z", args.z_min, args.z_max),
    ("floor hue", args.floor_hue_min, args.floor_hue_max),
    ("spot theta", args.spot_theta_min, args.spot_theta_max),
    ("spot phi", args.spot_phi_min, args.spot_phi_max),
    ("spot hue", args.spot_hue_min, args.spot_hue_max)
]

for name, minimum, maximum in ranges:
    if minimum > maximum:
        parser.error(f"Minimum {name} value cannot be greater than maximum")

#----------INITILISATION----------#

np.random.seed(args.seed)
bproc.init()

#----------VARIABLE SETUP----------#

output_dir = args.output_dir
os.makedirs(output_dir, exist_ok=True)

#----------LOADING DATA MODELS----------#

# Load the object list used by the EquiCaps dataset
objects = np.load("data/all_objects.npy", allow_pickle=True)

# Filter objects by the selected ShapeNet category
category_objects = objects[objects[:, 0] == args.model_category]

# Only include models that are available locally
available_models = [
    str(obj[1])
    for obj in category_objects
    if os.path.isdir(os.path.join("data", args.model_category, str(obj[1])))
]

if not available_models:
    raise RuntimeError(f"No models found locally for category {args.model_category}.")

# Select a specific model if an ID was provided, otherwise select randomly
if args.model_id is not None:
    if args.model_id not in available_models:
        raise RuntimeError(f"Model {args.model_id} is not available locally.")
    
    selected_object = args.model_id
else:
    selected_object = np.random.choice(available_models)

print(f"Selected model: {selected_object}")
print(f"Selected category: {args.model_category}")

# Load the selected ShapeNet model
model = bproc.loader.load_shapenet("data", used_synset_id=args.model_category, used_source_id=selected_object)

# Move the model origin to the centre of its bounding box
bbox = model.get_bound_box()
centre = np.mean(bbox, axis=0)
model.set_origin(point=centre)
model.set_location((0, 0, 0))

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

#----------TRANSFORMATION APPLICATION FUNCTION----------#

# Applying Transformations using Matrix Basis
def apply_matrix_basis_transformation(obj, translation=(0, 0, 0), rotation=(0, 0, 0)):
    translation_matrix = Matrix.Translation(translation)
    rotation_matrix = Euler(rotation).to_matrix().to_4x4()
    transformation_matrix = rotation_matrix @ translation_matrix
    obj.matrix_basis = transformation_matrix

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

    # Randomising the Floor Colour with a Randomly Generated Hue
    floor_hue = np.random.uniform(args.floor_hue_min, args.floor_hue_max)
    hsv = (floor_hue, 0.6, 0.6)
    rgb = matplotlib.colors.hsv_to_rgb(hsv)

    # Update the Floor Material's Base Colour
    bsdf.inputs["Base Color"].default_value = (*rgb, 1)

    # Randomly Generated Spotlight Position
    spot_theta = np.random.uniform(args.spot_theta_min, args.spot_theta_max)
    spot_phi = np.random.uniform(args.spot_phi_min, args.spot_phi_max)

    spot_location = spherical_to_cartesian(4, spot_theta, spot_phi)

    # Pointing the Spotlight Towards the Origin
    rotation_matrix = bproc.camera.rotation_from_forward_vec(np.array([0, 0, 0]) - spot_location)

    # Spotlight Transformation Matrix
    light_pose = Matrix(bproc.math.build_transformation_mat(spot_location, rotation_matrix))
    light.blender_obj.matrix_world = light_pose

    # Randomly Generated Spotlight Colours
    spot_hue = np.random.uniform(args.spot_hue_min, args.spot_hue_max)
    hsv = (spot_hue, 1, 0.8)
    rgb = matplotlib.colors.hsv_to_rgb(hsv)
    light.set_color(rgb)

    # Randomly generate the object's translation
    x = np.random.uniform(args.x_min, args.x_max)
    y = np.random.uniform(args.y_min, args.y_max)
    z = np.random.uniform(args.z_min, args.z_max)

    translation = (x, y, z)

    # Randomly Generated Rotational Angles.
    yaw = np.random.uniform(args.yaw_min, args.yaw_max)
    pitch = np.random.uniform(args.pitch_min, args.pitch_max)
    roll = np.random.uniform(args.roll_min, args.roll_max)

    rotation = (yaw, pitch, roll)
    
    # Applying transformations using matrix_basis
    apply_matrix_basis_transformation(model.blender_obj, translation, rotation)

    # Recording Transformation Data for This Image
    transformation_data.append([i, yaw, pitch, roll, floor_hue, x, y, z, spot_theta, spot_phi, spot_hue])

    # Creating the 10-Dimensional Latent Vector
    latent_vector = np.array([yaw, pitch, roll, floor_hue, spot_theta, spot_phi, spot_hue, x, y, z])

    # Saving the latent vector for this image
    np.save(os.path.join(output_dir, f"latent_{i}.npy"), latent_vector)

    # Rendering the Scene
    data = bproc.renderer.render()

    # Save Image
    image = data["colors"][0]
    Image.fromarray(image).save(os.path.join(output_dir, f"image_{i}.jpg"))

    print(f"Generated image {i + 1}/", generation_quantity)

#----------SAVE TRANSFORMATION DATA----------#

# Saving the Transformation Parameters into a CSV File
with open(os.path.join(output_dir, "transformations.csv"), "w", newline = "") as file:
    writer = csv.writer(file)

    writer.writerow(["image", "yaw", "pitch", "roll", "floor_hue", "x", "y", "z", "spot_theta", "spot_phi", "spot_hue"])
    writer.writerows(transformation_data)

#----------SAVE GENERATION METADATA----------#

# Store the Settings used to Generate the Dataset into a JSON File
metadata = {
    "seed": args.seed,

    "model": {
        "category": args.model_category,
        "model_id": selected_object
    },

    "generation": {
        "num_images": args.num_images,
        "image_size": args.image_size,
        "camera_distance": args.camera_distance
    },

    "rotation_ranges": {
        "yaw": [args.yaw_min, args.yaw_max],
        "pitch": [args.pitch_min, args.pitch_max],
        "roll": [args.roll_min, args.roll_max]
    },

    "translation_ranges": {
        "x": [args.x_min, args.x_max],
        "y": [args.y_min, args.y_max],
        "z": [args.z_min, args.z_max]
    },

    "lighting_ranges": {
        "floor_hue": [args.floor_hue_min, args.floor_hue_max],
        "spot_theta": [args.spot_theta_min, args.spot_theta_max],
        "spot_phi": [args.spot_phi_min, args.spot_phi_max],
        "spot_hue": [args.spot_hue_min, args.spot_hue_max]
    }
}

# Save metadata as a JSON file
with open(os.path.join(output_dir, "metadata.json"), "w") as file:
    json.dump(metadata, file, indent=4)

#----------TERMINAL OUTPUT----------#

print("Generation metadata saved.")
print("Completed with no errors.")