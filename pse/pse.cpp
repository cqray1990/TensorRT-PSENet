//
//  pse
//  reference https://github.com/whai362/PSENet/issues/15
//  Created by liuheng on 11/3/19. Modified by upczww.
//  Copyright © 2019年 liuheng. All rights reserved.
//
#include <queue>
#include "include/pybind11/pybind11.h"
#include "include/pybind11/numpy.h"
#include "include/pybind11/stl.h"
#include "include/pybind11/stl_bind.h"

namespace py = pybind11;

namespace pse
{
    //S5->S0, small->big
    py::array_t<int32_t> pse(
        py::array_t<int32_t, py::array::c_style> label_map,
        py::array_t<uint8_t, py::array::c_style> Sn,
        int c = 6)
    {
        auto pbuf_label_map = label_map.request();
        auto pbuf_Sn = Sn.request();
        if (pbuf_label_map.ndim != 2 || pbuf_label_map.shape[0] == 0 || pbuf_label_map.shape[1] == 0)
            throw std::runtime_error("label map must have a shape of (h>0, w>0)");
        int h = pbuf_label_map.shape[0];
        int w = pbuf_label_map.shape[1];
        if (pbuf_Sn.ndim != 3 || pbuf_Sn.shape[0] != c || pbuf_Sn.shape[1] != h || pbuf_Sn.shape[2] != w)
            throw std::runtime_error("Sn must have a shape of (c>0, h>0, w>0)");

        py::array_t<int32_t> out = py::array_t<int32_t>(h * w);
        out.resize({h, w});

        py::buffer_info buf_out = out.request();
        int *ptr_out = (int *)buf_out.ptr;
        memset(ptr_out, 0, h * w * sizeof(int32_t));

        auto ptr_label_map = static_cast<int32_t *>(pbuf_label_map.ptr);
        auto ptr_Sn = static_cast<uint8_t *>(pbuf_Sn.ptr);
        std::queue<std::tuple<int, int, int32_t>> q, next_q;

        for (int i = 0; i < h; i++)
        {
            auto p_label_map = ptr_label_map + i * w;
            for (int j = 0; j < w; j++)
            {
                int32_t label = p_label_map[j];
                if (label > 0)
                {
                    q.push(std::make_tuple(i, j, label));
                    ptr_out[i * w + j] = label;
                }
            }
        }

        int dx[4] = {-1, 1, 0, 0};
        int dy[4] = {0, 0, -1, 1};
        for (int i = c - 2; i >= 0; i--)
        {
            //get each kernels
            auto p_Sn = ptr_Sn + i * h * w;
            while (!q.empty())
            {
                //get each queue menber in q
                auto q_n = q.front();
                q.pop();
                int y = std::get<0>(q_n);     //i
                int x = std::get<1>(q_n);     //j
                int32_t l = std::get<2>(q_n); //label
                //store the edge pixel after one expansion
                bool is_edge = true;
                for (int idx = 0; idx < 4; idx++)
                {
                    int index_y = y + dy[idx];
                    int index_x = x + dx[idx];
                    if (index_y < 0 || index_y >= h || index_x < 0 || index_x >= w)
                        continue;
                    if (!p_Sn[index_y * w + index_x] || ptr_out[index_y * w + index_x] > 0)
                        continue;
                    q.push(std::make_tuple(index_y, index_x, l));
                    ptr_out[index_y * w + index_x] = l;
                    is_edge = false;
                }
                if (is_edge)
                {
                    next_q.push(std::make_tuple(y, x, l));
                }
            }
            std::swap(q, next_q);
        }
        return out;
    }
} // namespace pse

PYBIND11_MODULE(pse, m)
{
    m.def("pse_cpp", &pse::pse, " re-implementation pse algorithm(cpp)", py::arg("label_map"), py::arg("Sn"), py::arg("c") = 6);
}
