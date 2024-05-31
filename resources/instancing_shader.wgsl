
const UNIFORM_CULLING_PLANE = 1u;
const INSTANCE_UNLIT = 1u;
const INSTANCE_NO_CULLING_PLANE = 2u;
struct VertexInput {
	@location(0) position: vec3f,
	@location(1) normal: vec3f,
	@location(2) world_pos: vec3f,
	@location(3) rotation: vec4f, // a quaternion
	@location(4) scale: vec3f,
	@location(5) color: vec4f,
    @location(6) id: u32,
    @location(7) flags: u32
};

struct VertexOutput {
	@builtin(position) position: vec4f,
	@location(0) color: vec4f,
	@location(1) normal: vec3f,
    @location(2) worldpos: vec3f,
    @location(3) id: u32,
    @location(4) flags: u32
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
    flags: u32,
};

@group(0) @binding(0) var<uniform> uMyUniforms: MyUniforms;


fn quatDot(q1: vec4f, q2: vec4f) -> vec4f {
    let scalar = q1.w * q2.w - dot(q1.xyz, q2.xyz);
    let v = cross(q1.xyz, q2.xyz) + q1.w * q2.xyz + q2.w * q1.xyz;
    return vec4f(v, scalar);
}

fn quatInv(q: vec4f) -> vec4f {
    return vec4f(-q.xyz, q.w);
}

fn quatMul(q: vec4f, v: vec3f) -> vec3f {
    let r = quatDot(q, quatDot(vec4f(v, 0.0), quatInv(q)));
    return r.xyz;
}

fn transform(position: vec3f, rotation: vec4f, scale: vec3f, v: vec3f) -> vec3f {
    return position + quatMul(rotation, v * scale);
}


@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;

    let worldpos = transform(in.world_pos, in.rotation, in.scale, in.position);

    out.position = uMyUniforms.projectionMatrix * uMyUniforms.viewMatrix * vec4f(worldpos, 1.0);
    out.normal = quatMul(in.rotation, in.normal);
    out.color = in.color;
    out.id = in.id;
    out.flags = in.flags;
    out.worldpos = worldpos;
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {

    if (uMyUniforms.flags & UNIFORM_CULLING_PLANE) != 0u && (in.flags & INSTANCE_NO_CULLING_PLANE) == 0u {
        let distance = uMyUniforms.cullingDistance - dot(in.worldpos, uMyUniforms.cullingNormal);
        if distance < 0.0 {
        discard;
        }
    }
    // Basic phong shading
    var color = in.color.xyz;

    if (in.flags & INSTANCE_UNLIT) == 0u {

        let light_dir = normalize(vec3f(2.0, 0.5, 1.0));
        let normal = normalize(in.normal);
        let diffuse = max(dot(normal, light_dir), 0.0);
        let ambient = 0.1;
        color = color * (diffuse + ambient);
    }
	// Gamma-correction
    let corrected_color = pow(color, vec3f(2.2));
    return vec4f(corrected_color, in.color.w);
}