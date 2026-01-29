project('elad-spectrum', 'c',
  version: '1.0.0',
  default_options: ['c_std=c11', 'warning_level=2']
)

gtk4_dep = dependency('gtk4')
libusb_dep = dependency('libusb-1.0')
fftw3_dep = dependency('fftw3')
threads_dep = dependency('threads')
math_dep = meson.get_compiler('c').find_library('m', required: true)

src_files = [
  'src/main.c',
  'src/usb_device.c',
  'src/fft_processor.c',
  'src/spectrum_widget.c',
  'src/waterfall_widget.c',
  'src/cat_control.c',
]

executable('elad-spectrum',
  src_files,
  dependencies: [gtk4_dep, libusb_dep, fftw3_dep, threads_dep, math_dep],
  install: true
)
