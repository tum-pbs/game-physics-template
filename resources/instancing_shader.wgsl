
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
    @location(2) worldpos: vec3f,
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
    cullingNormal: vec3f,
    cullingDistance: f32,
};

@group(0) @binding(0) var<uniform> uMyUniforms: MyUniforms;

fn rotationX(angle: f32) -> mat3x3f {
    return mat3x3f(1.0, 0.0, 0.0,
        0.0, cos(angle), -sin(angle),
        0.0, sin(angle), cos(angle));
}

fn rotationY(angle: f32) -> mat3x3f {
    return mat3x3f(cos(angle), 0.0, sin(angle),
        0.0, 1.0, 0.0,
        -sin(angle), 0.0, cos(angle));
}

fn rotationZ(angle: f32) -> mat3x3f {
    return mat3x3f(cos(angle), -sin(angle), 0.0,
        sin(angle), cos(angle), 0.0,
        0.0, 0.0, 1.0);
}

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    let rotX = rotationX(radians(in.rotation.x));
    let rotY = rotationY(radians(in.rotation.y));
    let rotZ = rotationZ(radians(in.rotation.z));
    let rot = rotZ * rotY * rotX;

    out.position = uMyUniforms.projectionMatrix * uMyUniforms.viewMatrix * vec4f(rot * (in.position * in.scale) + in.world_pos, 1.0);
    out.normal = rot * in.normal;
    out.color = in.color;
    out.worldpos = rot * (in.position * in.scale) + in.world_pos;
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {

    if uMyUniforms.cullingDistance >= 0.0 {
        let distance = uMyUniforms.cullingDistance - dot(in.worldpos, uMyUniforms.cullingNormal);
        if distance < 0.0 {
        discard;
        }
    }
    // Basic phong shading

    let light_dir = normalize(vec3f(2.0, 0.5, 1.0));
    let normal = normalize(in.normal);
    let diffuse = max(dot(normal, light_dir), 0.0);
    let ambient = 0.1;
    let color = in.color * (diffuse + ambient);

	// Gamma-correction
    let corrected_color = pow(color, vec3f(2.2));
    return vec4f(corrected_color, 1.0);
}