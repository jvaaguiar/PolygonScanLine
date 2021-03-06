#include "polygondrawer.h"
#include "canvasopengl.h"
#include <algorithm>
#include <iostream>

#include "cgutils.h"

// ==================================================================================================
// PUBLIC MEMBERS
// ==================================================================================================
PolygonDrawer::PolygonDrawer(CanvasOpenGL* canvas, LightSource* light, Camera* camera) :
    Drawer(canvas), light(light), camera(camera), shading(Shading::FLAT) {}

// ==================================================================================================
PolygonDrawer::~PolygonDrawer() {}

// ==================================================================================================
void PolygonDrawer::Draw(QColor paintColor) {
    //Aplica o ScanLine apenas para 2 ou mais pontos
    if (Vertices.size() < 3) { return; }


    vector<vector<int>> zbuffer(static_cast<size_t>(canvas->width()));
    for(size_t i = 0; i < zbuffer.size(); i++)
        zbuffer[i].resize(static_cast<size_t>(canvas->height()), 10000000); // todo: camera far / near

    auto meshData = preparePoints();
    auto faces = meshData.first;
    auto normals = meshData.second;

    for (auto face : faces)
        switch (shading) {
        case Shading::FLAT :
            oddEvenFillMethodFLAT(face, paintColor, zbuffer);
            break;
        case Shading::GOURAUD :
            oddEvenFillMethodGOURAULD(face, normals, paintColor, zbuffer);
            break;
        case Shading::PHONG:
            oddEvenFillMethodPHONG(face, normals, paintColor, zbuffer);
        }
}

// ==================================================================================================
void PolygonDrawer::SetShading(PolygonDrawer::Shading shading) {
    this->shading = shading;
}

// ==================================================================================================
// PRIVATE MEMBERS
// ==================================================================================================
map<int, list<BlocoET> > PolygonDrawer::prepareEt(vector<QVector3D *> &vertices) {
    map<QVector3D*, QVector3D> mock;
    QColor col;
    return prepareEt(vertices, mock, col);
}

// ==================================================================================================
map<int, list<BlocoET>> PolygonDrawer::prepareEt(vector<QVector3D*>& vertices,
                                                 map<QVector3D*, QVector3D>& normals,
                                                 QColor& paintColor) {
    map<int, list<BlocoET>> et;
    auto n = vertices.size();
    for (size_t i = 0; i < n; i++) {
        // a -> b
        auto a = vertices[i];
        auto b = vertices[(i+1) % n];

        if (static_cast<int>(a->y()) == static_cast<int>(b->y())) { continue; }
        if (a->y() > b->y()) {
            auto swap = a;
            a = b;
            b = swap;
        }

        if (shading == Shading::GOURAUD) {
            auto aColor = shade(*a, normals[a], paintColor);
            auto bColor = shade(*b, normals[b], paintColor);

            BlocoET aux(static_cast<int>(a->y()), static_cast<int>(b->y()),
                        static_cast<int>(a->x()), static_cast<int>(b->x()),
                        static_cast<int>(a->z()), static_cast<int>(b->z()),
                        aColor.red(), bColor.red(),
                        aColor.green(), bColor.green(),
                        aColor.blue(), bColor.blue());

            et[static_cast<int>(a->y())].push_back(aux);
        }
        else if (shading == Shading::PHONG) {
            BlocoET aux(static_cast<int>(a->y()), static_cast<int>(b->y()),
                        static_cast<int>(a->x()), static_cast<int>(b->x()),
                        static_cast<int>(a->z()), static_cast<int>(b->z()),
                        normals[a], normals[b]);

            et[static_cast<int>(a->y())].push_back(aux);
        }
        else {
            BlocoET aux(static_cast<int>(a->y()), static_cast<int>(b->y()),
                        static_cast<int>(a->x()), static_cast<int>(b->x()),
                        static_cast<int>(a->z()), static_cast<int>(b->z()));

            et[static_cast<int>(a->y())].push_back(aux);
        }
    }
    return et;
}

// ==================================================================================================
void PolygonDrawer::updateAET (int y, list<BlocoET>& aet, map<int, list<BlocoET>>& et) {
    //Remove todos os pontos cujo y = ymax
    aet.remove_if([y](const BlocoET val) { return val.ymax == y; });

    //Transfere os valores da ET na posicao y para a AET
    auto etvals = et[y];
    aet.insert(aet.end(), etvals.begin(), etvals.end());
    et.erase(y);

    //Ordena se necessário
    aet.sort([](const BlocoET &b1, const BlocoET &b2) { return (b1.x < b2.x); });
}

// ==================================================================================================
QColor PolygonDrawer::shade(QVector3D& point, QVector3D& normal, QColor& paintColor) {
    // Lighting
    auto view = camera->GetPosition();
    auto fullLight = light->FullLighting(point, normal, view, shininess);
    auto diff = cteAmb + cteDiff * fullLight.first;
    auto spec = cteSpec * fullLight.second;
    QColor out;
    out.setRgbF(clamp01(diff * paintColor.redF() + spec),
                clamp01(diff * paintColor.greenF() + spec),
                clamp01(diff * paintColor.blueF() + spec));
    return out;
}

// ==================================================================================================
QColor PolygonDrawer::flatColor(QVector3D &n, QColor &c) {
    auto l = QVector3D(0, 0, -1);   // view direction
    auto cosTheta = clamp01(QVector3D::dotProduct(n, l));
    return QColor(static_cast<int>(c.red() * cosTheta),
                  static_cast<int>(c.green() * cosTheta),
                  static_cast<int>(c.blue() * cosTheta));
}

// ==================================================================================================
// returns all faces of the polyedre
std::pair<vector<vector<QVector3D*>>,
map<QVector3D*, QVector3D>> PolygonDrawer::preparePoints() {
    vector<vector<QVector3D*>> faces;
    map<QVector3D*, QVector3D> normals;

    // front and back
    vector<QVector3D*> front;
    vector<QVector3D*> back;
    for (auto v : Vertices) {
        front.push_back(new QVector3D(v->x(), v->y(), -this->extrusion));
        back.push_back(new QVector3D(v->x(), v->y(), this->extrusion));
    }

    auto frontNormal = QVector3D::normal(*front[0] - *front[1], *front[2] - *front[1]);
    if (frontNormal.z() > 0) {
        reverse(front.begin(), front.end());
        reverse(back.begin(), back.end());
    }

    for (auto v : front)
        normals[v] = frontNormal / 3;

    auto backNormal = -frontNormal;
    for (auto v : back)
        normals[v] = backNormal / 3;

    size_t n = Vertices.size();
    for (size_t i = 0; i < n; i++) {
        vector<QVector3D*> face = {back[i], back[(i+1)%n], front[(i+1)%n], front[i]};
        faces.push_back(face);

        auto normal = QVector3D::normal(*face[0] - *face[1], *face[2] - *face[1]);
        for(auto v : face)
            normals[v] += normal / 3;
    }

    faces.push_back(front);
    reverse(back.begin(), back.end());
    faces.push_back(back);

    // transform all points
    QMatrix4x4 t1;
    t1.translate(-canvas->width()/2, -canvas->height()/2, 0);
    QMatrix4x4 t2;
    t2.translate(canvas->width()/2, canvas->height()/2, 0);
    QMatrix4x4 rot;
    auto rotation = camera->GetRotation();
    rot.rotate(-rotation.x(), 1, 0, 0);
    rot.rotate(-rotation.y(), 0, 1, 0);
    rot.rotate(-rotation.z(), 0, 0, 1);

    for (size_t i = 0; i < front.size(); i++) {
        (*front[i]) = t1 * (*front[i]);
        (*front[i]) = rot * (*front[i]);
        (*front[i]) = t2 * (*front[i]);

        (*back[i]) = t1 * (*back[i]);
        (*back[i]) = rot * (*back[i]);
        (*back[i]) = t2 * (*back[i]);
    }

    return make_pair(faces, normals);
}

// ==================================================================================================
void PolygonDrawer::oddEvenFillMethodFLAT(vector<QVector3D*>& vertices,
                                          QColor& paintColor,
                                          vector<vector<int>>& zbuffer) {
    // Lighting
    QColor diffColor = paintColor;
    auto normal = QVector3D::normal(*vertices[0] - *vertices[1], *vertices[2] - *vertices[1]);
    diffColor = flatColor(normal, diffColor);

    QPainter painter(canvas);
    QPen myPen(diffColor);
    painter.setPen(myPen);

    // Inicializa a ET e a AET
    auto et = prepareEt(vertices);
    list<BlocoET> aet;

    // starts from min y in the polygon
    int y = static_cast<int>(vertices[0]->y());
    for (auto v : vertices)
        if (v->y() < y)
            y = static_cast<int>(v->y());
    int width = canvas->width();
    int height = canvas->height();

    while ((!et.empty() || !aet.empty()) && y < height) {
        updateAET(y, aet, et);

        //Desenha as linhas e incrementa os valores de x para a proxima iteracao
        auto it = aet.begin();
        while (it != aet.end()) {
            // 1st line
            auto x_beg = static_cast<int>(ceil(it->x));
            auto z_beg = static_cast<int>(ceil(it->z));
            it->x += it->mx;
            it->z += it->mz;
            it++;

            // 2nd line
            auto x_end = static_cast<int>(ceil(it->x));
            auto z_end = static_cast<int>(ceil(it->z));
            it->x += it->mx;
            it->z += it->mz;
            it++;

            if (y < 0) continue;

            // Z-BUFFER
            int x_init_z = -1;
            int x_end_z = -1;

            int x = x_beg < 0 ? 0 : ( x_beg >= width ? width - 1 : x_beg );

            if (x >= x_end) continue;

            double dz_dx = static_cast<double>(z_end - z_beg)/static_cast<double>(x_end - x_beg);
            double z = 1.0*z_beg + static_cast<double>(x - x_beg) * dz_dx;

            while(x < x_end && x < width) {
                if (zbuffer[static_cast<size_t>(x)][static_cast<size_t>(y)] > z) {
                    if (x_init_z < 0) x_init_z = x;
                    x_end_z = x;
                    zbuffer[static_cast<size_t>(x)][static_cast<size_t>(y)] = static_cast<int>(z);
                }
                else  {
                    if (x_init_z != -1)
                        painter.drawLine(x_init_z, y, x_end_z, y);
                    x_init_z = -1;
                }

                x++;
                z += dz_dx;
            }

            if (x_init_z != -1)
                painter.drawLine(x_init_z, y, x_end_z, y);
        }

        y++;
    }
}

// ==================================================================================================
void PolygonDrawer::oddEvenFillMethodGOURAULD(vector<QVector3D*>& vertices,
                                      map<QVector3D*, QVector3D>& normals,
                                      QColor& paintColor,
                                      vector<vector<int>>& zbuffer) {
    // Lighting
    QColor diffColor = paintColor;
    QPainter painter(canvas);
    QPen myPen(diffColor);
    painter.setPen(myPen);

    // Inicializa a ET e a AET
    auto et = prepareEt(vertices, normals, paintColor);
    list<BlocoET> aet;

    // starts from min y in the polygon
    int y = static_cast<int>(vertices[0]->y());
    for (auto v : vertices)
        if (v->y() < y)
            y = static_cast<int>(v->y());
    int width = canvas->width();
    int height = canvas->height();

    while ((!et.empty() || !aet.empty()) && y < height) {
        updateAET(y, aet, et);

        //Desenha as linhas e incrementa os valores de x para a proxima iteracao
        auto it = aet.begin();
        while (it != aet.end()) {
            // 1st line
            auto x_beg = static_cast<int>(ceil(it->x));
            auto z_beg = static_cast<int>(ceil(it->z));
            QColor col_beg (static_cast<int>(it->r), static_cast<int>(it->g), static_cast<int>(it->b));
            it->x += it->mx;
            it->z += it->mz;
            it->r += it->mr;
            it->g += it->mg;
            it->b += it->mb;
            it++;

            // 2nd line
            auto x_end = static_cast<int>(ceil(it->x));
            auto z_end = static_cast<int>(ceil(it->z));
            QColor col_end (static_cast<int>(it->r), static_cast<int>(it->g), static_cast<int>(it->b));
            it->x += it->mx;
            it->z += it->mz;
            it->r += it->mr;
            it->g += it->mg;
            it->b += it->mb;
            it++;

            if (y < 0) continue;


            // setup gradient in line
            QLinearGradient linearGrad(x_beg, y, x_end, y);
            QVector3D start (x_beg, y, 0);
            QVector3D end (x_end, y, 0);
            linearGrad.setColorAt(0, col_beg);
            linearGrad.setColorAt(1, col_end);
            QPen myPen(diffColor);
            QBrush brush(linearGrad);
            myPen.setBrush(brush);
            painter.setPen(myPen);


            // Z-BUFFER
            int x_init_z = -1;
            int x_end_z = -1;

            int x = x_beg < 0 ? 0 : ( x_beg >= width ? width - 1 : x_beg );

            if (x >= x_end) continue;

            double dz_dx = static_cast<double>(z_end - z_beg)/static_cast<double>(x_end - x_beg);
            double z = 1.0*z_beg + static_cast<double>(x - x_beg) * dz_dx;

            while(x < x_end && x < width) {
                if (zbuffer[static_cast<size_t>(x)][static_cast<size_t>(y)] > z) {
                    if (x_init_z < 0) x_init_z = x;
                    x_end_z = x;
                    zbuffer[static_cast<size_t>(x)][static_cast<size_t>(y)] = static_cast<int>(z);
                }
                else  {
                    if (x_init_z != -1)
                        painter.drawLine(x_init_z, y, x_end_z, y);
                    x_init_z = -1;
                }

                x++;
                z += dz_dx;
            }

            if (x_init_z != -1)
                painter.drawLine(x_init_z, y, x_end_z, y);
        }

        y++;
    }
}

// ==================================================================================================
void PolygonDrawer::oddEvenFillMethodPHONG(vector<QVector3D *> &vertices,
                                           map<QVector3D *, QVector3D> &normals,
                                           QColor &paintColor,
                                           vector<vector<int> > &zbuffer) {
    // Lighting
    QColor diffColor = paintColor;
    QPainter painter(canvas);
    QPen myPen(diffColor);
    painter.setPen(myPen);

    // Inicializa a ET e a AET
    auto et = prepareEt(vertices, normals, paintColor);
    list<BlocoET> aet;

    // starts from min y in the polygon
    int y = static_cast<int>(vertices[0]->y());
    for (auto v : vertices)
        if (v->y() < y)
            y = static_cast<int>(v->y());
    int width = canvas->width();
    int height = canvas->height();

    while ((!et.empty() || !aet.empty()) && y < height) {
        updateAET(y, aet, et);

        //Desenha as linhas e incrementa os valores de x para a proxima iteracao
        auto it = aet.begin();
        while (it != aet.end()) {
            // 1st line
            auto x_beg = static_cast<int>(ceil(it->x));
            auto z_beg = it->z;
            auto n_beg = it->n;
            it->x += it->mx;
            it->z += it->mz;
            it->n += it->mn;
            it++;

            // 2nd line
            auto x_end = static_cast<int>(ceil(it->x));
            auto z_end = it->z;
            auto n_end = it->n;
            it->x += it->mx;
            it->z += it->mz;
            it->n += it->mn;
            it++;

            // Z-BUFFER
            if (y < 0) continue;

            // Z-BUFFER
            int x = x_beg < 0 ? 0 : ( x_beg >= width ? width - 1 : x_beg );

            if (x >= x_end) continue;

            auto dx = 1.0*(x_end - x_beg);
            auto dx_1 = 1 / dx;
            auto dz_dx = static_cast<double>(z_end - z_beg)/static_cast<double>(x_end - x_beg);
            auto dn_dx = (n_end - n_beg) / static_cast<float>(dx);

            auto d = 0.0;
            auto z = 1.0*z_beg + static_cast<double>(x - x_beg) * dz_dx;
            auto n = 1.0*n_beg;

            while(x < x_end && x < width) {
                if (zbuffer[static_cast<size_t>(x)][static_cast<size_t>(y)] > static_cast<int>(z)) {
                    zbuffer[static_cast<size_t>(x)][static_cast<size_t>(y)] = static_cast<int>(z);
                    QVector3D point (x, y, static_cast<int>(z));
                    auto color = shade(point, n, paintColor);
                    QPen myPen(color);
                    painter.setPen(myPen);
                    painter.drawPoint(x, y);
                }

                x++;
                z += dz_dx;
                n += dn_dx;
                d += dx_1;
            }
        }

        y++;
    }
}
