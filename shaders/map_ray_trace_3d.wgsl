struct Index {
  pos: u32,
  offset: u32
};

const NULL_POS: u32 = ~0u;
const NULL_INDEX: Index = Index(NULL_POS, 0u);

struct Hit {
  node: Index,
  distance: f32
};

struct Sample {
  step_size: u32,
  offset: u32
};

struct Uniform {
  projection: mat4x4f,
  view: mat4x4f,
  dim: vec2u,
  near_clip: f32,
  far_clip: f32,
  @size(16) sample: Sample,
  @size(16) node: Index,
  node_center: vec3f,
  node_half_length: f32,
};

struct Code {
  code: array<u32, 3>,
  depth: u32,
};

struct TreeBlock {
  children: array<u32, 8>,
  parent: u32,
  _pad: u32,
  code: Code,
  modified: array<u32, 2>
};

struct OccupancyBlock {
  occupancy: array<u32, 4>
};

struct ColorBlock {
  color: array<u32, 8>
};

@group(0) @binding(0) var<storage,read_write> hits_node: array<Hit>;
@group(0) @binding(1) var<storage,read_write> hits_depth: array<Hit>;
@group(0) @binding(2) var<uniform> uniforms: Uniform;
@group(0) @binding(3) var<storage,read> tree_buffer: array<TreeBlock>;
@group(0) @binding(4) var<storage,read> occupancy_buffer: array<OccupancyBlock>;

struct Ray {
  origin: vec3f,
  direction: vec3f
};

struct TraceParams {
  node: Index,
  t0: vec3f,
  t1: vec3f,
  a: u32
};

fn min3(v: vec3f) -> f32 {
  return min(min(v.x, v.y), v.z);
}

fn minIndex(v: vec3f) -> u32 {
  let my = u32((v.y < v.z) && (v.y < v.x));
  let mz = 2u * u32((v.z < v.y) && (v.z < v.x));
  return my + mz;
}

fn max3(v: vec3f) -> f32 {
  return max(max(v.x, v.y), v.z);
}

fn maxIndex(v: vec3f) -> u32 {
  let my = u32((v.y > v.z) && (v.y > v.x));
  let mz = 2u * u32((v.z > v.y) && (v.z > v.x));
  return my + mz;
}

fn firstNode(tm: vec3f, t: f32) -> u32 {
  return u32(tm[0] < t) | (u32(tm[1] < t) << 1u) | (u32(tm[2] < t) << 2u);
}

fn newNode(cur: u32, dim: u32) -> u32 {
  // return ((cur << (3u - dim)) & 8u) | cur | (1u << dim);
  let x = 1u << dim;
  return ((cur & x) << (3u - dim)) | cur | x;
}

fn children(node: Index) -> u32 {
  return tree_buffer[node.pos].children[node.offset];
}

fn child(node: Index, child: u32) -> Index {
  return Index(children(node), child);
}

fn isLeaf(node: Index) -> bool {
  return ~0u == children(node);
}

fn depth(node: Index) -> u32 {
  return tree_buffer[node.pos].code.depth;
}

fn hasChildren(node: Index) -> bool {
  return !isLeaf(node);
}

fn containsUnknown(node: Index) -> bool {
  return 0u != (occupancy_buffer[node.pos].occupancy[node.offset / 2u] & (1u << (16u * (node.offset % 2u))));
}

fn containsFree(node: Index) -> bool {
  return 0u != (occupancy_buffer[node.pos].occupancy[node.offset / 2u] & (2u << (16u * (node.offset % 2u))));
}

fn containsOccupied(node: Index) -> bool {
  return 0u != (occupancy_buffer[node.pos].occupancy[node.offset / 2u] & (4u << (16u * (node.offset % 2u))));
}

fn occupied(node: Index) -> bool {
  return 0.0 < unpack4x8snorm(occupancy_buffer[node.pos].occupancy[node.offset / 2u])[2u * (node.offset % 2u)];
}

fn returnable(node: Index) -> bool {
  return isLeaf(node) && occupied(node);
}

fn traversable(node: Index) -> bool {
  return hasChildren(node) && containsOccupied(node);
}

fn offset(code: Code, depth: u32) -> u32 {
  let i = depth / 10u;
  let d = depth % 10u;
  // let c = code.code[i];
  let c = select(code.code[0], select(code.code[1], code.code[2], 1 != i), 0 != i);
  return (c >> (3u * d)) & 7u;
}

fn parent(node: Index) -> Index {
  let d = depth(node) + 1;
  let o = offset(tree_buffer[node.pos].code, d);
  let p = tree_buffer[node.pos].parent;
  return Index(p, o);
}

fn trace(params: TraceParams) -> Hit {
  var node = params.node;
  var t0   = params.t0;
  var t1   = params.t1;
  var tm   = (t0 + t1) * vec3f(0.5);
  let a    = params.a;

  let max_t0 = max3(t0);
  let min_t1 = min3(t1);

  let min_dist = max(0.0, max_t0);
  let max_dist = min_t1;

  if max_t0 >= min_t1 || uniforms.near_clip > max_dist || uniforms.far_clip < min_dist {
    return Hit(NULL_INDEX, -1.0);
  } else if uniforms.near_clip <= max_dist && returnable(node) {
    return Hit(node, min_dist);
  } else if !traversable(node) {
    return Hit(NULL_INDEX, 1.0);
  }

  // 32 / (3 + 4) = 4
  // 19 depth levels -> 4 * 5 = 20, so we need at least 5, for 19 depth levels

  var stack: array<u32, 8>;
  stack[0] = firstNode(tm, max_t0);

  for (var index = 0; 0 <= index;) {
    var bucket =  u32(index) / 4u;
    var pos    = (u32(index) % 4u) * 7u;

    var next_offset = (stack[bucket] >> pos) & 15u;

    if 8u <= next_offset {
      index--;

      node = parent(node);

      let m = (t1 - t0);
      let min = t0 - m;
      let max = t1 + m;

      let prev_offset = (stack[bucket] >> (pos + 4u)) & 7u;

      let mask = vec3<bool>(bool(prev_offset & 1u), 
                            bool(prev_offset & 2u), 
                            bool(prev_offset & 4u));

      t0 = select(t0, min, mask);
      t1 = select(max, t1, mask);
      tm = (t0 + t1) * vec3f(0.5);

      continue;
    }

    let child = child(node, next_offset ^ a);

    let mask = vec3<bool>(bool(next_offset & 1u), 
                          bool(next_offset & 2u), 
                          bool(next_offset & 4u));

    let child_t0 = select(t0, tm, mask);
    let child_t1 = select(tm, t1, mask);

    let min_t1_idx = minIndex(child_t1);

    let prev_offset = next_offset;
    next_offset     = newNode(next_offset, min_t1_idx);
    
    stack[bucket] &= ~(15u << pos);
    stack[bucket] |= next_offset << pos;

    let max_child_t0 = max3(child_t0);
    let min_dist     = max(0.0, max_child_t0);
    let max_dist     = child_t1[min_t1_idx];

    if uniforms.near_clip > max_dist {
      continue;
    } else if uniforms.far_clip < min_dist {
      return Hit(NULL_INDEX, -1.0);
    } else if uniforms.near_clip <= max_dist && returnable(child) {
      return Hit(child, min_dist);
    } else if !traversable(child) {
      continue;
    }
    
    index++;

    node = child;
    t0   = child_t0;
    t1   = child_t1;
    tm   = (t0 + t1) * vec3f(0.5);

    bucket         =  u32(index) / 4u;
    pos            = (u32(index) % 4u) * 7u;
    stack[bucket] &= ~(127u << pos);
    next_offset    = firstNode(tm, max_child_t0);
    stack[bucket] |= (next_offset | (prev_offset << 4u)) << pos;
  }

  return Hit(NULL_INDEX, -1.0);
}

fn traceInit(node: Index, ray: Ray) -> TraceParams {
  let mask = vec3f(0.0) > ray.direction;

  let origin = select(ray.origin, (uniforms.node_center * 2.0) - ray.origin, mask);
  let dir    = select(ray.direction, -ray.direction, mask);

  let t0_t = uniforms.node_center - uniforms.node_half_length - origin;
  let t1_t = uniforms.node_center + uniforms.node_half_length - origin;

  let dir_reciprocal = select(vec3f(1.0) / dir, vec3f(1e+25), vec3f(0.0) == dir);

  let t0 = t0_t * dir_reciprocal;
  let t1 = t1_t * dir_reciprocal;

  let a = u32(mask[0]) | (u32(mask[1]) << 1u) | (u32(mask[2]) << 2u);

  return TraceParams(node, t0, t1, a);
}

fn samplePixel(id: vec2u) -> vec2u {
  return vec2u(
    id.x * uniforms.sample.step_size + (uniforms.sample.offset % uniforms.sample.step_size),
    id.y * uniforms.sample.step_size + (uniforms.sample.offset / uniforms.sample.step_size)
  );
}

fn isOffScreen(v: vec2u) -> bool {
  return any(uniforms.dim <= v);
}

// TODO: Use pipeline-overridable constants when implemented in WGPU
// override block_width = 8u;
// override block_height = 4u;
// @compute @workgroup_size(block_width, block_height)
@compute @workgroup_size(8, 4)
fn main(@builtin(global_invocation_id) id: vec3u) {
  let pixel = samplePixel(id.xy);

  if isOffScreen(pixel) {
    return;
  }

  let texel = vec2f(pixel) + vec2f(0.5);

  let texel_nds = (texel / vec2f(uniforms.dim)) * vec2f(2.0) - vec2f(1.0);
  let p_nds_h = vec4f(texel_nds.x, texel_nds.y, -1.0, 1.0);
  var dir_eye = uniforms.projection * p_nds_h;
  dir_eye.w = 0.0;
  let dir_world = normalize((uniforms.view * dir_eye).xyz);
  // let pos_world = (uniforms.view * vec4f(vec3f(0.0), 1.0)).xyz;
  let pos_world = uniforms.view[3].xyz;

  let ray  = Ray(pos_world, dir_world);

  let params = traceInit(uniforms.node, ray);

  // TODO: What should this be?
  hits[pixel.x + pixel.y * uniforms.dim.x] = trace(params);
}
