/* See Copyright Notice in LICENSE.txt */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "freetype-gl.h"


#include <GL/gl.h>
//#include <FTGL/ftgl.h>
#include <lauxlib.h>
#include <lualib.h>

#include "misc.h"
#include "shader.h"
#include "vertex-buffer.h"

typedef struct {
    float x, y, z;    // position
    float s, t;       // texture
    float r, g, b, a; // color
} vertex_t;

typedef struct {
    texture_font_t *font;
    texture_atlas_t *atlas;
    const char *text;
    vertex_buffer_t *buffer;
} font_t;

typedef struct {
    const char *text;
    vertex_buffer_t *buffer;
} cache_entry_t;


LUA_TYPE_DECL(font)

/* Instance methods */
#define SCALE (72)
void font_buffer_new (font_t *font, const char *text);

static int font_write(lua_State *L) {
    font_t *font = checked_font(L, 1);
    GLfloat x = luaL_checknumber(L, 2);
    GLfloat y = luaL_checknumber(L, 3);
    const char *text = luaL_checkstring(L, 4);
    vertex_buffer_t *buffer = NULL;

    // Protect FTGL
    if (!check_utf8(text))
        return luaL_error(L, "invalid utf8");

    GLfloat size = luaL_checknumber(L, 5) / SCALE;
    fprintf (stderr, "%s x = %g y = %g text %s size = %g\n", __func__, x, y, text, size);
    /*int type = lua_type(L, 6);
    if (type == LUA_TNUMBER) {
        GLfloat r = luaL_checknumber(L, 6);
        GLfloat g = luaL_checknumber(L, 7);
        GLfloat b = luaL_checknumber(L, 8);
        GLfloat a = luaL_optnumber(L, 9, 1.0);

        shader_set_gl_color(r, g, b, a);
        glBindTexture(GL_TEXTURE_2D, default_tex);
    } else if (type == LUA_TUSERDATA || type == LUA_TTABLE) {
        lua_pushliteral(L, "texid");
        lua_gettable(L, 6);
        if (lua_type(L, -1) != LUA_TFUNCTION)
            return luaL_argerror(L, 6, "no texid() function");
        lua_pushvalue(L, 6);
        lua_call(L, 1, 1);
        if (lua_type(L, -1) != LUA_TNUMBER)
            return luaL_argerror(L, 6, "texid() did not return number");
        int tex_id = lua_tonumber(L, -1);
        lua_pop(L, 1);

        shader_set_gl_color(1.0, 1.0, 1.0, 1.0);
        glBindTexture(GL_TEXTURE_2D, tex_id);
    } else {
        return luaL_argerror(L, 6, "unsupported value. must be RGBA or texturelike");
    }*/

    if (!font->buffer)
        font_buffer_new (font, text);


    glPushMatrix();
        glTranslatef(x, y, 0);
        glTranslatef(0, size * (SCALE * 0.8), 0);
        glScalef(size, -size, 1.0);
        //ftglRenderFont(font->font, text, FTGL_RENDER_ALL);
        vertex_buffer_render( font->buffer, GL_TRIANGLES );
    glPopMatrix();

    //lua_pushnumber(L, ftglGetFontAdvance(font->font, text) * size);
    return 1;
}

static int font_width(lua_State *L) {
    font_t *font = checked_font(L, 1);
    const char *text = luaL_checkstring(L, 2);
    GLfloat size = luaL_checknumber(L, 3) / SCALE;
    //lua_pushnumber(L, ftglGetFontAdvance(font->font, text) * size);
    return 1;
}

static const luaL_reg font_methods[] = {
    {"write",       font_write},
    {"width",       font_width},
    {0,0}
};

void font_search_text (const char *text) {

}

void add_text( vertex_buffer_t * buffer, texture_font_t * font,
               wchar_t * text, vec4 * color, vec2 * pen )
{
    size_t i;
    float r = color->red, g = color->green, b = color->blue, a = color->alpha;
    for( i=0; i<wcslen(text); ++i )
    {
        texture_glyph_t *glyph = texture_font_get_glyph( font, text[i] );
        if( glyph != NULL )
        {
            float kerning =  0.0f;
            if( i > 0)
            {
                kerning = texture_glyph_get_kerning( glyph, text[i-1] );
            }
            pen->x += kerning;
            int x0  = (int)( pen->x + glyph->offset_x );
            int y0  = (int)( pen->y + glyph->offset_y );
            int x1  = (int)( x0 + glyph->width );
            int y1  = (int)( y0 - glyph->height );
            float s0 = glyph->s0;
            float t0 = glyph->t0;
            float s1 = glyph->s1;
            float t1 = glyph->t1;
            GLuint indices[6] = {0,1,2, 0,2,3};
            vertex_t vertices[4] = { { x0,y0,0,  s0,t0,  r,g,b,a },
                                     { x0,y1,0,  s0,t1,  r,g,b,a },
                                     { x1,y1,0,  s1,t1,  r,g,b,a },
                                     { x1,y0,0,  s1,t0,  r,g,b,a } };
            vertex_buffer_push_back( buffer, vertices, 4, indices, 6 );
            pen->x += glyph->advance_x;
        }
    }
}

/* Lifecycle */
void font_buffer_new (font_t *font, const char *text) {
    vec2 pen = {{5,400}};
    vec4 black = {{0,0,0,1}};

    pen.x = 5;
    pen.y -= font->font->height;
    font->text = text;

    texture_font_load_glyphs( font->font, (wchar_t*) text );
    font->buffer = vertex_buffer_new( "vertex:3f,tex_coord:2f,color:4f" );
    add_text( font->buffer, font->font, (wchar_t*) text, &black, &pen );
    //texture_font_delete( font );
}

int font_new(lua_State *L, const char *path, const char *name) {
    //FTGLfont *ftgl_font = ftglCreateTextureFont(path);
    font_t newfont;
    texture_atlas_t *atlas = texture_atlas_new( 512, 512, 1 );
    newfont.atlas = atlas;
    //newfont->path = path;
    newfont.font = texture_font_new_from_file( atlas, 12, path );
    newfont.text = NULL;
    newfont.buffer = NULL;

    /*for( i=7; i < 27; ++i)
    {
        font = texture_font_new_from_file( atlas, 12, path );
        if (!font)
            return luaL_error(L, "cannot load font file %s", path);
        pen.x = 5;
        pen.y -= font->height;
        //texture_font_load_glyphs( font, text );
        //buffer = vertex_buffer_new( "vertex:3f,tex_coord:2f,color:4f" );
        //add_text( buffer, font, text, &black, &pen );
        texture_font_delete( font );
    }*/

    //ftglSetFontDisplayList(ftgl_font, 1);
    //ftglSetFontFaceSize(ftgl_font, SCALE, SCALE);
    //ftglSetFontCharMap(ftgl_font, ft_encoding_unicode);

    font_t *font = push_font(L);
    *font = newfont;
    return 1;
}

static int font_gc(lua_State *L) {
    font_t *font = to_font(L, 1);
    //ftglDestroyFont(font->font);
    fprintf(stderr, INFO("gc'ing font\n"));
    return 0;
}

LUA_TYPE_IMPL(font)
