// Fragment shader: sample R8 texture and output grayscale RGBA.

$input v_texcoord0

#include <bgfx_shader.sh>

SAMPLER2D(s_texColor, 0);

void main()
{
  vec4 c = texture2D(s_texColor, v_texcoord0);
  // If texture is R8, value will be in .r; replicate to RGB, set alpha to 1.
  gl_FragColor = vec4(c.rrr, 1.0);
}
