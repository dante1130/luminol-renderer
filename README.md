# luminol-renderer

A simple renderer using OpenGL. This project is still a WIP.

Features:

- Physically based rendering
- Deferred shading
- Light casters
  - Directional light
  - 64 point lights
  - 64 spot lights
- Model loading
- Skybox
- HDR
- First person camera

## Index

* [Prerequisites](#prerequisites)
* [Cloning the repository](#cloning)
* [Rendering samples](#rendering-samples)

## Prerequisites

You need to install the following:

- [Git](https://git-scm.com/downloads)
- [CMake](https://cmake.org/download/) (minimum version of 3.25.1)
- [Python](https://www.python.org/downloads/) (This is used for `glad` to generate the OpenGL loader)

You will also need the `Jinja2` Python package. You can install it using pip by running this command:

```bash
pip install jinja2
```

## Cloning the repository

In a terminal with Git installed, clone the repository by typing in the following command in a directory where you want your project:

```bash
$ git clone https://github.com/dante1130/luminol-renderer
```

## Rendering samples

![backpack-img](https://i.imgur.com/45Uv648.png)
![fish-img](https://i.imgur.com/000SIRB.png)
![katana-img](https://i.imgur.com/LliKnfI.png)
![geisha-img](https://i.imgur.com/JiF8oLw.png)

