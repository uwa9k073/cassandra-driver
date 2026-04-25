FROM ubuntu:24.04 AS builder

WORKDIR /docs

COPY . .

RUN apt update -y && apt install git graphviz doxygen -y

RUN git submodule update --init third_party/doxygen-awesome-css.git

RUN doxygen scripts/docs/doxygen.conf

FROM nginx:1.28-alpine

COPY --from=builder /docs/docs/html /usr/share/nginx/html

COPY scripts/docs/nginx.conf /etc/nginx/conf.d/default.conf

RUN ls -la /usr/share/nginx/html

ENTRYPOINT ["nginx", "-g", "daemon off;"]
