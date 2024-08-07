# Fluid.c
This is a simple 2D fluid simulator, implemented from scratch in C.<br>
It simulates the `Navier-Stokes` equations in a \*stable\* way. [[refs](#references)]<br>
With this program, you can simulate fluid dynamics in real-time, based on the given `viscosity` and `diffusion` rate.

<br>

| ![preview1](https://github.com/user-attachments/assets/23fd4cbd-2b72-4c40-aed3-71ada6cdf620) |
|:--:|
| *0% viscosity, 0% diffusion* |

| ![preview2](https://github.com/user-attachments/assets/7a5d0ff9-6ba5-4086-a8fb-f91eafdbc83f) |
|:--:|
| *70% viscosity, 0% diffusion* |

| ![preview3](https://github.com/user-attachments/assets/d1663aa9-85a0-458f-bc78-b57648f6849c) |
|:--:|
| *0% viscosity, 70% diffusion* |

<br>

## Supported Platforms
- Windows 10 x64, version 1607 or later

<br>

## Program Usage
| Action           | Description                                       |
|------------------|---------------------------------------------------|
|Mouse left-click  | Add density to the fluid.                         |
|Mouse move        | Add force to the fluid in moving direction.       |
|`←` key           | Decrease the diffusion rate of the fluid.         |
|`→` key           | Increase the diffusion rate of the fluid.         |
|`↑` key           | Increase the viscosity of the fluid.              |
|`↓` key           | Decrease the viscosity of the fluid.              |
|`F1` key          | Toggle render mode. (color/gray)                  |
|`F12` key         | Toggle verbose mode.                              |

<br>

## References
- [Stam, Jos. (2001). Stable Fluids. ACM SIGGRAPH 99. 1999. 10.1145/311535.311548.](https://www.researchgate.net/publication/2486965_Stable_Fluids)
- [Stam, Jos. (2003). Real-Time Fluid Dynamics for Games.](https://www.researchgate.net/publication/2560062_Real-Time_Fluid_Dynamics_for_Games)
