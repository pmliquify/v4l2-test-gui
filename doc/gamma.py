# plot_srgb_gamma.py
import numpy as np
import matplotlib.pyplot as plt

# Wertebereich 0..1
x = np.linspace(0.0, 1.0, 1024)

# Approx. sRGB gamma (vereinfachte Potenz-Approximation 2.2)
srgb_to_linear = x ** 2.2       # sRGB -> linear (inverse gamma)
linear_to_srgb = x ** (1.0 / 2.2)  # linear -> sRGB (gamma encoding)

plt.figure(figsize=(6,4))
plt.plot(x, srgb_to_linear, label='sRGB -> linear (x^2.2)', color='C0')
plt.plot(x, linear_to_srgb, label='linear -> sRGB (x^(1/2.2))', color='C1', linestyle='--')

# einige Beispielpunkte markieren
samples = np.array([0.05, 0.2, 0.5, 0.8])
plt.scatter(samples, samples**2.2, color='C0', s=40)
plt.scatter(samples, samples**(1/2.2), color='C1', s=40)

plt.xlabel('Eingang (0..1)')
plt.ylabel('Ausgang (0..1)')
plt.title('sRGB / linear RGB: Gamma-Transformation (exponent ≈ 2.2)')
plt.grid(alpha=0.3)
plt.legend()
plt.tight_layout()
plt.show()