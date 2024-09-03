
struct VertexInput {
	@location(0) position: vec3f,
	@location(1) color: vec3f,
};

struct VertexOutput {
	@builtin(position) position: vec4f,
	@location(0) color: vec3f,
};

//
// A structure holding the value of our uniforms
//
struct RenderUniforms {
	projectionMatrix: mat4x4f,
	viewMatrix: mat4x4f,
	cameraWorldPosition: vec3f,
	time: f32,
	cullingOffsets: vec3f,
	flags: u32,
};

@group(0) @binding(0) var<uniform> renderUniforms: RenderUniforms;


@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;

    out.position = renderUniforms.projectionMatrix * renderUniforms.viewMatrix * vec4f(in.position, 1.0);
    out.color = in.color;
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {



	// Gamma-correction
    let corrected_color = pow(in.color, vec3f(2.2));
    return vec4f(corrected_color, 1.0);
}