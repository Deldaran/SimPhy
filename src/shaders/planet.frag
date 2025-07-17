#version 450 core

// --- Simplex Noise 3D (Ashima Arts) ---
vec3 mod289(vec3 x) {
  return x - floor(x * (1.0 / 289.0)) * 289.0;
}
vec4 mod289(vec4 x) {
  return x - floor(x * (1.0 / 289.0)) * 289.0;
}
vec4 permute(vec4 x) {
     return mod289(((x*34.0)+10.0)*x);
}
vec4 taylorInvSqrt(vec4 r)
{
  return 1.79284291400159 - 0.85373472095314 * r;
}
float snoise(vec3 v)
  { 
  const vec2  C = vec2(1.0/6.0, 1.0/3.0) ;
  const vec4  D = vec4(0.0, 0.5, 1.0, 2.0);
  vec3 i  = floor(v + dot(v, C.yyy) );
  vec3 x0 =   v - i + dot(i, C.xxx) ;
  vec3 g = step(x0.yzx, x0.xyz);
  vec3 l = 1.0 - g;
  vec3 i1 = min( g.xyz, l.zxy );
  vec3 i2 = max( g.xyz, l.zxy );
  vec3 x1 = x0 - i1 + C.xxx;
  vec3 x2 = x0 - i2 + C.yyy; 
  vec3 x3 = x0 - D.yyy;      
  i = mod289(i); 
  vec4 p = permute( permute( permute( 
             i.z + vec4(0.0, i1.z, i2.z, 1.0 ))
           + i.y + vec4(0.0, i1.y, i2.y, 1.0 )) 
           + i.x + vec4(0.0, i1.x, i2.x, 1.0 ));
  float n_ = 0.142857142857; 
  vec3  ns = n_ * D.wyz - D.xzx;
  vec4 j = p - 49.0 * floor(p * ns.z * ns.z);  
  vec4 x_ = floor(j * ns.z);
  vec4 y_ = floor(j - 7.0 * x_ );    
  vec4 x = x_ *ns.x + ns.yyyy;
  vec4 y = y_ *ns.x + ns.yyyy;
  vec4 h = 1.0 - abs(x) - abs(y);
  vec4 b0 = vec4( x.xy, y.xy );
  vec4 b1 = vec4( x.zw, y.zw );
  vec4 s0 = floor(b0)*2.0 + 1.0;
  vec4 s1 = floor(b1)*2.0 + 1.0;
  vec4 sh = -step(h, vec4(0.0));
  vec4 a0 = b0.xzyw + s0.xzyw*sh.xxyy ;
  vec4 a1 = b1.xzyw + s1.xzyw*sh.zzww ;
  vec3 p0 = vec3(a0.xy,h.x);
  vec3 p1 = vec3(a0.zw,h.y);
  vec3 p2 = vec3(a1.xy,h.z);
  vec3 p3 = vec3(a1.zw,h.w);
  vec4 norm = taylorInvSqrt(vec4(dot(p0,p0), dot(p1,p1), dot(p2, p2), dot(p3,p3)));
  p0 *= norm.x;
  p1 *= norm.y;
  p2 *= norm.z;
  p3 *= norm.w;
  vec4 m = max(0.5 - vec4(dot(x0,x0), dot(x1,x1), dot(x2,x2), dot(x3,x3)), 0.0);
  m = m * m;
  return 105.0 * dot( m*m, vec4( dot(p0,x0), dot(p1,x1), 
                                dot(p2,x2), dot(p3,x3) ) );
  }

// --- fBm ---
float fbm(vec3 p) {
    float value = 0.0;
    float amplitude = 1.0;
    float frequency = 0.2;
    for (int i = 0; i < 3; ++i) {
        value += amplitude * snoise(p * frequency);
        frequency *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}
float fbm(vec2 p) {
    return fbm(vec3(p, 0.0));
}

// --- Masque pour plaines et montagnes ---
float terrainMask(vec3 p) {
    // Bruit lent pour placer les montagnes (valeurs proches de 0 = plaine, proches de 1 = montagne)
    return smoothstep(0.3, 0.7, fbm(p * 0.05));
}

float triplanar_terrain(vec3 p) {
    float base = fbm(p * 0.2); // Grandes plaines
    float mask = terrainMask(p); // Masque montagne
    float mountain = fbm(p * 1.0); // Détail montagne
    return base + mask * mountain * 2.0; // Amplifie les montagnes là où le masque est fort
}
out vec4 FragColor;

uniform float planetRadius;
uniform vec3 planetCenter;

in VS_OUT {
    vec3 pos;
    vec3 normal;
    vec2 uv;
} te_out;


// --- Simplex Noise 3D (Ashima Arts) ---

// --- SDF pour la planète ---
float planetSDF(vec3 p) {
    float h = triplanar_terrain(p) * 20.0; // hauteur max 20m, relief varié
    return length(p - planetCenter) - (planetRadius + h);
}

// --- Gradient pour la normale ---
vec3 estimateNormal(vec3 p) {
    float eps = 0.5; // Plus grand pour moins de lag
    return normalize(vec3(
        planetSDF(p + vec3(eps,0,0)) - planetSDF(p - vec3(eps,0,0)),
        planetSDF(p + vec3(0,eps,0)) - planetSDF(p - vec3(0,eps,0)),
        planetSDF(p + vec3(0,0,eps)) - planetSDF(p - vec3(0,0,eps))
    ));
}

void main() {
    // Position et normale interpolées
    vec3 pos = te_out.pos;
    // Normale via gradient SDF
    vec3 normal = estimateNormal(pos);

    // Lumière
    vec3 lightDir = normalize(vec3(10, 10, 10));
    float diff = max(dot(normal, lightDir), 0.0);
    float spec = pow(max(dot(reflect(-lightDir, normal), normalize(pos)), 0.0), 32.0);
    vec3 baseColor = vec3(0.2, 0.5, 0.8);
    FragColor = vec4(baseColor * diff + vec3(0.5) * spec, 1.0);
}
