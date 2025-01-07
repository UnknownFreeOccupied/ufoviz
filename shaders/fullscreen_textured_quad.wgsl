@group(0) @binding(0) var texture_sampler : sampler;
@group(0) @binding(1) var texture_buffer : texture_2d<f32>;
@group(0) @binding(2) var<storage,read> depth_buffer: array<f32>;

struct VertexOutput {
  @builtin(position) pos : vec4f,
  @location(0) uv : vec2f,
}

struct FragmentOutput {
  @builtin(frag_depth) depth: f32,
  @location(0) color: vec4<f32>,
}

@vertex
fn vertMain(@builtin(vertex_index) id : u32) -> VertexOutput {
  // TODO: Should be const
  var pos = array(
    vec2( 1.0,  1.0),
    vec2( 1.0, -1.0),
    vec2(-1.0, -1.0),
    vec2( 1.0,  1.0),
    vec2(-1.0, -1.0),
    vec2(-1.0,  1.0),
  );

  // TODO: Should be const
  var uv = array(
    vec2(1.0, 1.0),
    vec2(1.0, 0.0),
    vec2(0.0, 0.0),
    vec2(1.0, 1.0),
    vec2(0.0, 0.0),
    vec2(0.0, 1.0),
  );

  return VertexOutput(vec4(pos[id], 0.0, 1.0), uv[id]);
}

fn gammaCorrect(c: vec4f) -> vec4f {
  return pow(c, vec4f(2.2));
}

fn toSRGB(c: vec4f) -> vec4f {
  let greaterThanZero = vec3f(0.0) < c.rgb;
  let underLowThreshold = vec3f(0.0031308) > c.rgb;
  let greaterThanOrEqualToOne = vec3f(1.0) <= c.rgb;
  let rgb = select(
     vec3f(0.0),
     select(
       c.rgb * vec3f(12.92),
       select(
          pow(vec3f(1.055) * c.rgb, vec3f(0.41666)) - vec3f(0.055),
          vec3f(1.0),
          greaterThanOrEqualToOne,
       ),
       underLowThreshold,
     ),
     greaterThanZero,
  );
  return vec4f(select(c.rgb, vec3f(0.0), c.rgb <= vec3f(0.0)), c.a);
}

fn fromSRGB(c: vec4f) -> vec4f {
  let overLowThreshold = vec3f(0.04045) < c.rgb;
  let rgb = select(c.rgb / vec3f(12.92), pow((c.rgb + vec3f(0.055)) / vec3f(1.055), vec3f(2.4)), overLowThreshold);
  return vec4f(saturate(rgb), c.a);
}

@fragment
fn fragMain(@location(0) uv : vec2f) -> FragmentOutput {
  // TODO: Implement
  return FragmentOutput(0.0, gammaCorrect(textureSample(texture_buffer, texture_sampler, uv)));
}