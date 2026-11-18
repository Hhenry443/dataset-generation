
import blenderproc as bproc
import numpy as np
import math
import os

bproc.init()

# The image will save to an output directory as a hdf5 file.
output_dir = "output" 

os.makedirs("output", exist_ok=True)

cube = bproc.object.create_primitive("CUBE")
cube.set_scale([1, 1, 1])

# Creating the Camera

# Creating the Camera
bproc.camera.set_resolution(512, 512)

# Position the camera
camera_location = np.array([3, -3, 2])

# Calculate the direction from the camera towards the origin
forward_vec = -camera_location

# Calculate the camera rotation so it looks towards the object
camera_rotation = bproc.camera.rotation_from_forward_vec(
    forward_vec,
    "Z"
)

# Combine the camera position and rotation
camera_pose = bproc.math.build_transformation_mat(
    camera_location,
    camera_rotation
)

bproc.camera.add_camera_pose(camera_pose)


#lighting
light = bproc.types.Light()
light.set_type("POINT")
light.set_location([3, -3, 5])
light.set_energy(1000)

for i in range(10):
    # Random rotation
    yaw = np.random.uniform(-math.pi / 2, math.pi / 2)
    pitch = np.random.uniform(-math.pi / 2, math.pi / 2)
    roll = np.random.uniform(-math.pi / 2, math.pi / 2)

    cube.set_rotation_euler([pitch, roll, yaw])

    # Random translations
    translation = np.random.uniform(-0.5, 0.5, 3)
    cube.set_location(translation)

    # Rendering the Scene
    data = bproc.renderer.render()

    # Save image
    image = data["colors"][0]

    from PIL import Image
    Image.fromarray(image).save(
        os.path.join(output_dir, f"image_{i}.png")
    )

    print(f"Generated image {i + 1}/10")


print("Completed with no errors.")
#Run through terminal