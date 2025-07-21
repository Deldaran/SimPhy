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

// --- Masque de continents ---
float continentMask(vec3 p) {
    // Bruit très lent pour définir les continents (valeurs proches de 0 = océan, proches de 1 = terre)
    return smoothstep(0.35, 0.55, fbm(p * 0.01));
}

float triplanar_terrain(vec3 p) {
    float continent = continentMask(p); // 0 = océan, 1 = terre
    float base = fbm(p * 0.2) * continent; // Grandes plaines sur la terre
    float mask = terrainMask(p) * continent; // Masque montagne sur la terre
    float mountain = fbm(p * 1.0) * continent; // Détail montagne sur la terre
    return mix(-0.2, base + mask * mountain * 2.0, continent); // Océan = -0.2, Terre = relief
}
out vec4 FragColor;


uniform float planetRadius;
uniform vec3 planetCenter;
uniform vec3 cameraPos;
uniform vec3 sunDirection; // direction du soleil


in VS_OUT {
    vec3 pos;
    vec3 normal;
    vec2 uv;
} te_out;


// --- Simplex Noise 3D (Ashima Arts) ---

// --- SDF pour la planète ---
float planetSDF(vec3 p) {
    // Utilise l'altitude réelle du mesh CPU (interpolée par le rasterizer)
    float h = te_out.uv.y * 20.0; // 20.0 = même échelle que le mesh CPU
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

// --- Biome selection ---
int getBiome(float altitude, float lat, float moisture) {
    if (altitude < 0.0) return 0; // océan (toute altitude négative)
    if (altitude < 0.03) return 1; // plage (bande plus fine)
    if (moisture > 0.5 && altitude < 0.6 && lat < 0.7) return 2; // forêt
    if (altitude < 0.7) return 3; // plaine
    if (altitude < 0.85) return 4; // montagne
    return 5; // sommet neigeux
}

vec3 biomeColor(int biome) {
    if (biome == 0) return vec3(0.0, 0.2, 0.7); // océan
    if (biome == 1) return vec3(0.9, 0.8, 0.5); // plage
    if (biome == 2) return vec3(0.1, 0.6, 0.2); // forêt
    if (biome == 3) return vec3(0.5, 0.7, 0.3); // plaine
    if (biome == 4) return vec3(0.5, 0.5, 0.5); // montagne
    return vec3(1.0, 1.0, 1.0); // neige
}

// Ray marching parameters
#define MAX_STEPS 128
#define MIN_DIST 0.01
#define MAX_DIST 10000.0

// Ray marching to find intersection with planet SDF
bool rayMarch(vec3 ro, vec3 rd, out vec3 hitPos, out float hitDist) {
    float t = 0.0;
    for (int i = 0; i < MAX_STEPS; ++i) {
        vec3 p = ro + rd * t;
        float d = planetSDF(p);
        if (abs(d) < MIN_DIST) {
            hitPos = p;
            hitDist = t;
            return true;
        }
        t += d;
        if (t > MAX_DIST) break;
    }
    return false;
}

void main() {
    // Utilise la position et la normale du mesh CPU

    vec3 pos = te_out.pos;
    float altitude = te_out.uv.y;
    float lat = abs(normalize(pos - planetCenter).y);
    float moisture = fbm(pos * 0.03);
    int biome = getBiome(altitude, lat, moisture);
    vec3 baseColor = biomeColor(biome);

    // Normale : radiale partout (sphère parfaite)
    vec3 normal = normalize(pos - planetCenter);

    // Lumière étendue : direction = du point de la surface du soleil le plus proche
    float scaledRadius = planetRadius;
    vec3 scaledPlanetCenter = planetCenter;
    float sunDist = scaledRadius * 20000.0;
    float sunRadius = scaledRadius * 1000.0;
    vec3 sunCenter = scaledPlanetCenter + sunDirection * sunDist;
    vec3 scaledPos = pos;
    vec3 dirToSun = normalize(sunCenter - scaledPos);
    vec3 sunSurface = sunCenter - dirToSun * sunRadius;
    // Lumière principale : direction du soleil vers le point (pour tout objet)
    vec3 lightDir = normalize(pos - sunSurface);
    float diff = max(dot(normal, lightDir), 0.01);

    // Ombre portée du relief via ray marching
    float shadow = 1.0;
    vec3 shadowOrigin = sunSurface;
    vec3 shadowDir = normalize(pos - sunSurface);
    vec3 shadowHit; float shadowDist;
    if (rayMarch(shadowOrigin, shadowDir, shadowHit, shadowDist)) {
        float distToSurface = length(pos - sunSurface);
        float distToHit = length(shadowHit - sunSurface);
        if (distToHit < distToSurface - 2.0) shadow = 0.0;
    }

    // Blinn-Phong spéculaire
    vec3 viewDir = normalize(cameraPos - pos);
    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfDir), 0.0), 64.0);

    // Ambiant simple
    vec3 ambient = vec3(0.15);
    vec3 color = baseColor * (ambient + diff * shadow) + vec3(1.0) * spec * 0.2;

    // --- Lumière indirecte par rebond diffus (1 bounce lissé) ---
    // Direction lissée : bruit à basse fréquence et mix avec la normale
    //vec3 noiseVec = vec3(fbm(pos * 0.03), fbm(pos * 0.05 + 10.0), fbm(pos * 0.07 - 20.0));
    //vec3 randDir = normalize(mix(normal, normalize(normal + noiseVec), 0.3));
    //if (dot(randDir, normal) < 0.0) randDir = -randDir;
    //vec3 bounceOrigin = pos + normal * 2.0;
    //vec3 bounceHit; float bounceDist;
    //bool bounce = rayMarch(bounceOrigin, randDir, bounceHit, bounceDist);
    //if (bounce) {
    //    float bounceAlt = length(bounceHit - planetCenter) - planetRadius;
    //    float bounceLat = abs(normalize(bounceHit - planetCenter).y);
    //    float bounceMoist = fbm(bounceHit * 0.03);
    //    int bounceBiome = getBiome(bounceAlt / 20.0, bounceLat, bounceMoist);
    //    vec3 bounceColor = biomeColor(bounceBiome);
    //    vec3 bounceNormal = normalize(bounceHit - planetCenter);
    //    vec3 bounceToSun = normalize(sunSurface - bounceHit);
    //    float bounceDiff = max(dot(bounceNormal, bounceToSun), 0.0);
    //    color += 0.2 * bounceColor * bounceDiff;
    //}

    FragColor = vec4(color, 1.0);
}
