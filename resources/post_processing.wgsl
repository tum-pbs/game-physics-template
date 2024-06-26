struct VertexOutput {
        @builtin(position) position: vec4f,
        @location(0) uv: vec2f,
      }


@vertex
fn vs_main(@builtin(vertex_index) VertexIndex: u32) -> VertexOutput {
    var pos = array(
        vec2(1.0, 1.0),
        vec2(-1.0, -1.0),
        vec2(1.0, -1.0),
        vec2(1.0, 1.0),
        vec2(-1.0, 1.0),
        vec2(-1.0, -1.0),
    );

    var uv = array(
        vec2(1.0, 0.0),
        vec2(0.0, 1.0),
        vec2(1.0, 1.0),
        vec2(1.0, 0.0),
        vec2(0.0, 0.0),
        vec2(0.0, 1.0),
    );

    var output: VertexOutput;
    output.position = vec4(pos[VertexIndex], 0.0, 1.0);
    output.uv = uv[VertexIndex];
    return output;
}

@group(0) @binding(0) var postProTexture: texture_2d<f32>;
@group(0) @binding(1) var postProSampler: sampler;

@fragment
fn fs_main(@location(0) uv: vec2f) -> @location(0) vec4<f32> {
    let color = textureSample(postProTexture, postProSampler, uv);
    var effect = 1.0;
    var uv2 = uv * (1.0 - uv.yx);
    var vignette = uv2.x * uv2.y * 15.0;
    effect = pow(vignette, 0.5);
    return vec4f(color.rgb, 1.0);
}