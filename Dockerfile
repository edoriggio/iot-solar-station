# Use a base image with Miniconda installed
FROM continuumio/miniconda3

# Set the working directory in the container
WORKDIR /usr/src/app

# Copy the current directory contents into the container at /usr/src/app
COPY ./src/python ./python
COPY ./environment.yml .
COPY ./config.ini .

# Create a Conda environment using the environment.yml file
RUN conda env create -f environment.yml

# Activate the environment
SHELL ["conda", "run", "-n", "solar-station", "/bin/bash", "-c"]

# Ensure the environment is activated:
RUN echo "Make sure conda is used:" && \
    which python && \
    which pip

# The code to run when container is started:
CMD ["conda", "run", "-n", "solar-station", "python", "./python/server.py"]
