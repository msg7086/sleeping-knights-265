import vapoursynth as vs
core = vs.core
try:
    clip = core.ffms2.Source("14946398.mp4")
except Exception:
    clip = core.lsmas.LWLibavSource("14946398.mp4")
clip.set_output()
