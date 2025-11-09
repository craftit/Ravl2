// Vertex shader: pass-through position/texcoord for screen-quad rendering.
// No dependencies on bgfx includes to simplify shaderc invocation.

$input a_position, a_texcoord0
$output v_texcoord0

void main()
{
  // Positions are provided in NDC (-1..1). Z at 0, W at 1.
  gl_Position = vec4(a_position, 0.0, 1.0);
  v_texcoord0 = a_texcoord0;
}
