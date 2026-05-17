using System;
using System.Collections.Generic;
using System.Linq;
using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Audio;
using Microsoft.Xna.Framework.Content;
using Microsoft.Xna.Framework.GamerServices;
using Microsoft.Xna.Framework.Graphics;
using Microsoft.Xna.Framework.Input;
using Microsoft.Xna.Framework.Media;
using System.Reflection;
using Triton;

namespace TritonXNA
{
    /// <summary>
    /// This is the main type for your game
    /// </summary>
    public class Game1 : Microsoft.Xna.Framework.Game
    {
        GraphicsDeviceManager graphics;
        SpriteBatch spriteBatch;
        Triton.Ocean ocean;
        Triton.Environment environment;
        Triton.ResourceLoader resourceLoader;
        Matrix world, view, projection;
        VertexPositionColor[] vertices;
        BasicEffect effect;

        public Game1()
        {
            graphics = new GraphicsDeviceManager(this);
            Content.RootDirectory = "Content";
        }

        /// <summary>
        /// Allows the game to perform any initialization it needs to before starting to run.
        /// This is where it can query for any required services and load any non-graphic
        /// related content.  Calling base.Initialize will enumerate through any components
        /// and initialize them as well.
        /// </summary>
        protected override void Initialize()
        {
            // TODO: Add your initialization logic here
            world = Matrix.Identity;
            base.Initialize();
        }

        /// <summary>
        /// Extracts the underlying IDirect3DDevice9 from XNA, and initializes Triton with it.
        /// Also sets up the simulated conditions.
        /// </summary>
        unsafe private bool InitializeTriton()
        {
            // Grab the IDirect3DDevice9 from XNA using reflection. Doing this requires us to mark this method
            // as "unsafe."
            GraphicsDevice device = GraphicsDevice;
            FieldInfo field = typeof(GraphicsDevice).GetField("pComPtr", BindingFlags.NonPublic | BindingFlags.Instance);
            Pointer reflectionPointer = (Pointer)field.GetValue(device);
            void* voidPointer = Pointer.Unbox(reflectionPointer);
            System.IntPtr devicePtr = (System.IntPtr)voidPointer;

            // Initialize our resource loader with the path to our Triton resources
            // You could derive your own resource loader to work with your own resource management scheme.
            String tritonPath = System.Environment.GetEnvironmentVariable("TRITON_PATH");
            resourceLoader = new Triton.ResourceLoader(tritonPath + "/resources");

            // Create the environment for Triton
            environment = new Triton.Environment();
            environment.SetLicenseCode("Your user name", "Your license code");
            EnvironmentError err = environment.Initialize(CoordinateSystem.FLAT_YUP, Renderer.DIRECTX_9, resourceLoader, devicePtr);
            if (err == EnvironmentError.SUCCEEDED)
            {
                // And the Ocean:
                ocean = Triton.Ocean.Create(environment);

                // Set up some wind to give us some waves
                WindFetch windFetch = new WindFetch();
                windFetch.SetWind(5.0, 0.0);
                environment.AddWindFetch(windFetch);

                // Ready to use our Ocean object now.
                return true;
            }
            return false;
        }

        /// <summary>
        /// Set the view and projection matrices, and pass them in to SilverLining.
        /// </summary>
        private void SetMatrices()
        {

            // Just look straight ahead.
            view = Matrix.CreateLookAt(new Microsoft.Xna.Framework.Vector3(0, 10, 0),
                new Microsoft.Xna.Framework.Vector3(100.0f, 0.0f, 0.0f), 
                new Microsoft.Xna.Framework.Vector3(0, 1, 0));

            // Create the projection
            projection = Matrix.CreatePerspectiveFieldOfView(MathHelper.PiOver4, GraphicsDevice.Viewport.AspectRatio, 10.0f, 100000.0f);

            double[] m = new double[16];

            m[0] = projection.M11; m[1] = projection.M12; m[2] = projection.M13; m[3] = projection.M14;
            m[4] = projection.M21; m[5] = projection.M22; m[6] = projection.M23; m[7] = projection.M24;
            m[8] = projection.M31; m[9] = projection.M32; m[10] = projection.M33; m[11] = projection.M34;
            m[12] = projection.M41; m[13] = projection.M42; m[14] = projection.M43; m[15] = projection.M44;

            // Passing arrays across C# / C++ boundaries is a little tricky. Here's how we do it:
            SWIGTYPE_p_double matrix4 = TritonEnvironment.new_double_array(16);
            for (int i = 0; i < 16; i++)
            {
                TritonEnvironment.double_array_setitem(matrix4, i, m[i]);
            }
            environment.SetProjectionMatrix(matrix4);
            TritonEnvironment.delete_double_array(matrix4);

            m[0] = view.M11; m[1] = view.M12; m[2] = view.M13; m[3] = view.M14;
            m[4] = view.M21; m[5] = view.M22; m[6] = view.M23; m[7] = view.M24;
            m[8] = view.M31; m[9] = view.M32; m[10] = view.M33; m[11] = view.M34;
            m[12] = view.M41; m[13] = view.M42; m[14] = view.M43; m[15] = view.M44;

            matrix4 = TritonEnvironment.new_double_array(16);
            for (int i = 0; i < 16; i++)
            {
                TritonEnvironment.double_array_setitem(matrix4, i, m[i]);
            } 
            environment.SetCameraMatrix(matrix4);
            TritonEnvironment.delete_double_array(matrix4);
        }

        /// <summary>
        /// LoadContent will be called once per game and is the place to load
        /// all of your content.
        /// </summary>
        protected override void LoadContent()
        {
            // Create a new SpriteBatch, which can be used to draw textures.
            spriteBatch = new SpriteBatch(GraphicsDevice);

            // Set up our degenerate triangle, used to trigger a BeginScene call
            // for SilverLining inside Draw():
            SetUpVertices();
            effect = new BasicEffect(GraphicsDevice);

            // Set up Triton.
            InitializeTriton();
        }

        /// <summary>
        /// UnloadContent will be called once per game and is the place to unload
        /// all content.
        /// </summary>
        protected override void UnloadContent()
        {
            // TODO: Unload any non ContentManager content here
        }

        /// <summary>
        /// Allows the game to run logic such as updating the world,
        /// checking for collisions, gathering input, and playing audio.
        /// </summary>
        /// <param name="gameTime">Provides a snapshot of timing values.</param>
        protected override void Update(GameTime gameTime)
        {
            // Allows the game to exit
            if (GamePad.GetState(PlayerIndex.One).Buttons.Back == ButtonState.Pressed)
                this.Exit();

            // TODO: Add your update logic here

            base.Update(gameTime);
        }

        /// <summary>
        /// Sets up vertices for a degenerate triangle (used to trigger a BeginScene call for Triton
        /// later on).
        /// </summary>
        private void SetUpVertices()
        {
            vertices = new VertexPositionColor[3];

            vertices[0].Position = new Microsoft.Xna.Framework.Vector3(0.0f, 0.0f, 0.0f);
            vertices[0].Color = Color.Red;
            vertices[1].Position = new Microsoft.Xna.Framework.Vector3(0.0f, 0.0f, 0.0f);
            vertices[1].Color = Color.Green;
            vertices[2].Position = new Microsoft.Xna.Framework.Vector3(0.0f, 0.0f, 0.0f);
            vertices[2].Color = Color.Yellow;
        }

        /// <summary>
        /// This is called when the game should draw itself.
        /// </summary>
        /// <param name="gameTime">Provides a snapshot of timing values.</param>
        protected override void Draw(GameTime gameTime)
        {
            GraphicsDevice.Clear(Color.CornflowerBlue);

            SetMatrices();

            // Set lighting conditions
            environment.SetAmbientLight(new Triton.Vector3(0.3, 0.3, 0.3));
            environment.SetDirectionalLight(new Triton.Vector3(0, 1, 0), new Triton.Vector3(0.9, 0.9, 0.9));

            // If you have an environmental cube map, you can give it to Triton for proper water reflections.
            // Something along these lines; pass in the pComPtr property of Texture2D as we need the native
            // texture pointer.
            //environment.SetEnvironmentMap(cubeMap.pComPtr);

            effect.World = world;
            effect.View = view;
            effect.Projection = projection;

            // Update the ocean wave simulation. You could actually do this from another thread if you wanted to.
            if (ocean != null)
            {
                ocean.UpdateSimulation(gameTime.TotalGameTime.TotalSeconds);
            }

            foreach (EffectPass pass in effect.CurrentTechnique.Passes)
            {
                pass.Apply();

                // Triton under DX9 needs to be within a BeginScene/EndScene block of the underlying IDirect3DDevice9. We draw a 
                // degenerate triangle to trick XNA into entering a drawing block for us, but drawing anything prior to 
                // Ocean::Draw will do the job.
                GraphicsDevice.DrawUserPrimitives(PrimitiveType.TriangleList, vertices, 0, 1, VertexPositionColor.VertexDeclaration);

                // Now draw the ocean:
                if (ocean != null)
                {
                    ocean.Draw(gameTime.TotalGameTime.TotalSeconds);
                }
            }

            base.Draw(gameTime);
        }
    }
}
