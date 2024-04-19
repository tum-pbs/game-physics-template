
struct VertexInput {
	@location(0) position: vec3f,
	@location(1) normal: vec3f,
	@location(2) world_pos: vec3f,
	@location(3) rotation: vec3f,
	@location(4) scale: vec3f,
	@location(5) color: vec3f,
};

struct VertexOutput {
	@builtin(position) position: vec4f,
	@location(0) color: vec3f,
	@location(1) normal: vec3f,
};

//
// A structure holding the value of our uniforms
//
struct MyUniforms {
	projectionMatrix: mat4x4f,
	viewMatrix: mat4x4f,
	modelMatrix: mat4x4f,
	cameraWorldPosition: vec3f,
	time: f32,
};

@group(0) @binding(0) var<uniform> uMyUniforms: MyUniforms;

fn rotationX(angle: f32) -> mat4x4f {
    return mat4x4f(1.0, 0.0, 0.0, 0.0,
        0.0, cos(angle), -sin(angle), 0.0,
        0.0, sin(angle), cos(angle), 0.0,
        0.0, 0.0, 0.0, 1.0);
}

fn rotationY(angle: f32) -> mat4x4f {
    return mat4x4f(cos(angle), 0.0, sin(angle), 0.0,
        0.0, 1.0, 0.0, 0.0,
        -sin(angle), 0.0, cos(angle), 0.0,
        0.0, 0.0, 0.0, 1.0);
}

fn rotationZ(angle: f32) -> mat4x4f {
    return mat4x4f(cos(angle), -sin(angle), 0.0, 0.0,
        sin(angle), cos(angle), 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0);
}

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    let rotX = rotationX(radians(in.rotation.x));
    let rotY = rotationY(radians(in.rotation.y));
    let rotZ = rotationZ(radians(in.rotation.z));
    let rot = rotZ * rotY * rotX;

    let trans = mat4x4f(
        1.0, 0.0, 0.0, in.world_pos.x,
        0.0, 1.0, 0.0, in.world_pos.y,
        0.0, 0.0, 1.0, in.world_pos.z,
        0.0, 0.0, 0.0, 1.0
    );

    let scale = mat4x4f(
        in.scale.x, 0.0, 0.0, 0.0,
        0.0, in.scale.y, 0.0, 0.0,
        0.0, 0.0, in.scale.z, 0.0,
        0.0, 0.0, 0.0, 1.0
    );
    let model = rot * scale;

    out.position = uMyUniforms.projectionMatrix * uMyUniforms.viewMatrix * (model * vec4f(in.position, 1.0) + vec4f(in.world_pos, 0.0));
    out.normal = (model * vec4f(in.normal, 0.0)).xyz;
    out.color = in.color;
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {


    // Basic phong shading

    let light_dir = normalize(vec3f(1.0, 1.0, 1.0));
    let normal = normalize(in.normal);
    let diffuse = max(dot(normal, light_dir), 0.0);
    let ambient = 0.1;
    let color = in.color * (diffuse + ambient);

	// Gamma-correction
    let corrected_color = pow(color, vec3f(2.2));
    return vec4f(corrected_color, 1.0);
}