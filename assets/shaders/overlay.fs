#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Output fragment color
out vec4 finalColor;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Screen texture
uniform sampler2D underlayTexture;

vec3 rgbToHsl(vec3 color)
{
    float maxc = max(color.r, max(color.g, color.b));
    float minc = min(color.r, min(color.g, color.b));
    float l = (maxc + minc) * 0.5;
    float s, h;

    if (maxc == minc) {
        h = 0.0;
        s = 0.0;
    } else {
        float d = maxc - minc;
        s = (l > 0.5) ? d / (2.0 - maxc - minc) : d / (maxc + minc);

        if (maxc == color.r)
            h = (color.g - color.b) / d + (color.g < color.b ? 6.0 : 0.0);
        else if (maxc == color.g)
            h = (color.b - color.r) / d + 2.0;
        else
            h = (color.r - color.g) / d + 4.0;

        h /= 6.0;
    }

    return vec3(h, s, l);
}

vec3 hslToRgb(vec3 hsl)
{
    float h = hsl.x, s = hsl.y, l = hsl.z;
    float q = l < 0.5 ? l * (1.0 + s) : l + s - l * s;
    float p = 2.0 * l - q;

    vec3 rgb = vec3(
        clamp(h + 1.0 / 3.0, 0.0, 1.0),
        clamp(h, 0.0, 1.0),
        clamp(h - 1.0 / 3.0, 0.0, 1.0)
    );

    rgb = mix(vec3(p), vec3(q), step(0.5, rgb) * 2.0 * (rgb - 0.5) + (1.0 - step(0.5, rgb)) * 2.0 * rgb);

    return rgb;
}

vec3 getContrastingColor(vec3 color)
{
    float brightness = dot(color, vec3(0.299, 0.587, 0.114));
    return brightness < 0.5 ? vec3(0.8) : vec3(0.0);
}

void main()
{
    vec4 texelColor = texture(texture0, fragTexCoord);
    vec4 underlayColor = texture(underlayTexture, fragTexCoord);
    finalColor = vec4(getContrastingColor(underlayColor.rgb), texelColor.a);
}