struct UBO {
    projection: mat4x4f,
    view: mat4x4f,
    color: u32,
};

@group(0) @binding(0) var<uniform> uniforms: UBO;

@vertex
fn vertMain(@location(0) position: vec3f) -> @builtin(position) vec4f {
    return uniforms.projection * uniforms.view * vec4f(position, 1.0);
}

@fragment
fn fragMain() -> @location(0) vec4f {
    return unpack4x8unorm(uniforms.color);
}
