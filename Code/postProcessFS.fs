#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform int u_effect; // 0 = Normal, 1 = Inversion, 2 = Noir & Blanc, 3 = Flou

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
        default:
            break; // Normal
    }
    FragColor = vec4(col, 1.0);
}