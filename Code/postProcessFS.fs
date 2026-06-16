#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform int u_effect; // 0 = Normal, 1 = Inversion, 2 = Noir & Blanc, 3 = Flou, 4 = Sépia, 5 = Convolution, 6 = Vignettage, 7 = Aberration Chromatique, 8 = Pixellisation

vec3 inversion(vec3 col) {
    col = vec3(1.0) - col;
    return col;
}

vec3 NoirEtBlanc(vec3 col) {
    float gray = dot(col, vec3(0.299, 0.587, 0.114));
    col = vec3(gray);
    return col;
}

vec3 blur(vec3 col) {
    float offset = 1.0 / 300.0; // Adjust the offset for blur effect
    vec3 blurCol = vec3(0.0);
    for(int x = -1; x <= 1; x++) {
        for(int y = -1; y <= 1; y++) {
            blurCol += texture(screenTexture, TexCoords + vec2(x, y) * offset).rgb;
        }
    }
    col = blurCol / 9.0; // Average the colors
    return col;
}

vec3 sepia(vec3 col) {
    vec3 sepia = vec3(
        dot(col, vec3(0.4, 0.7, 0.2)),
        dot(col, vec3(0.3, 0.7, 0.2)),
        dot(col, vec3(0.3, 0.5, 0.1))
    );
    col = sepia; // Sépia
    return col;
}

vec3 convolutionKernel(vec3 col){
    ivec2 texSize = textureSize(screenTexture, 0);
    vec2 texOffset = vec2(1/float(texSize.x), 1/float(texSize.y));
    vec2 offsets[9] = vec2[](
        vec2(-texOffset.x, texOffset.y),
        vec2(0.0, texOffset.y),
        vec2(texOffset.x, texOffset.y),
        vec2(-texOffset.x, 0.0),
        vec2(0.0, 0.0),
        vec2(texOffset.x, 0.0),
        vec2(-texOffset.x, -texOffset.y),
        vec2(0.0, -texOffset.y),
        vec2(texOffset.x, -texOffset.y)
    );
    float kernel[9] = float[](-1, -1, -1, -1, 8, -1, -1, -1, -1);
    vec3 kCol = vec3(0);
    for(int i=0; i<9; i++) {
        vec3 tex = texture(screenTexture, TexCoords + offsets[i]).rgb;
        kCol += tex * kernel[i];
    }
    col = kCol; // noyau de convolution
    return col;
} 

vec3 vignettage(vec3 col) {
    float vignettage = smoothstep(0.8, 0.3, distance(TexCoords, vec2(0.5, 0.5)));
    col = vec3(vignettage); // Vignettage
    col *= (1.0 - vignettage);
    return col;
}

vec3 aberrationChromatique(vec3 col) {
    float offset = 0.005; // force
    float r = texture(screenTexture, TexCoords + vec2(offset, 0.0)).r;
    float g = texture(screenTexture, TexCoords).g;
    float b = texture(screenTexture, TexCoords - vec2(offset, 0.0)).b;
    col = vec3(r, g, b); // Aberration chromatique
    return col;
}
vec3 pixellisation(vec3 col) {
float pixels = 150.0; // quantité
    vec2 retro = floor(TexCoords * pixels) / pixels;
    col = texture(screenTexture, retro).rgb; // pixélisation
    return col;
}

void main() {
    vec3 col = texture(screenTexture, TexCoords).rgb;
    
    switch(u_effect) {
        case 1: // Inversion
            col = inversion(col);
            break;
        case 2: // Noir & Blanc
            col = NoirEtBlanc(col);
            break;
        case 3: // Flou (Blur)
            col = blur(col); // Average the colors
            break;
        case 4: //sepia
            col = sepia(col); // Sépia
            break;
        case 5: // convolution kernel
            col = convolutionKernel(col); // noyau de convolution
            break;
        case 6: // Aberration chromatique
            col = aberrationChromatique(col);
            break;
        case 7: // Pixellisation
            col = pixellisation(col);
            break;
        default:
            break; // Normal
    }
    FragColor = vec4(col, 1.0);
}