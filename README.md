# UNIMAN — EMG-controlled robotic hand

An open, 3D-printable robotic hand controlled by a surface EMG sensor.
Contract your forearm muscle and the printed hand closes.

It is an educational platform, not a prosthesis and not a medical
device — built to make the path from a muscle signal to a moving hand
something you can build yourself and understand at every step.

<img width="3024" height="4032" alt="IMG_5142" src="https://github.com/user-attachments/assets/08ae1b3c-6cfe-4db9-b1e7-30edaf6b0efd" />


## What it does

- Seven printed parts, fingers come off the printer already articulated
- One servo drives all five fingers through tendon cables
- Calibrates itself to whoever is wearing the electrode, in about
  thirty seconds
- Two control modes: hold to close, or toggle on each contraction

## How the control works

The sensor outputs a signal envelope proportional to muscle effort.
The firmware compares it against two thresholds — above the upper one
the hand closes, below the lower one it opens, and in between the state
is held. That gap is what keeps the hand from oscillating near the
switching point.

Both thresholds come from calibration rather than from constants,
because signal amplitude varies by a factor of two or three between
people.

## Repository layout

| Folder | Contents |
|---|---|
| `hardware/` | STL files for 3D printing |
| `firmware/` | Arduino sketch |
| `electronics/` | Wiring diagram |

## Limitations

Binary control, no force feedback, a single EMG channel, and testing on
a small number of people.

## Licence

See [LICENSE](LICENSE).

## Author

Piotr Sakowski

UNIMAN was presented as *True Motion: From muscle signal to robotic
hand motion*.

Email: piotrsakowski333@gmail.com  
LinkedIn: https://www.linkedin.com/in/sako-piotr/
